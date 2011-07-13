/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "GlideComputer.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "ConditionMonitor.hpp"
#include "TeamCodeCalculation.hpp"
#include "PeriodClock.hpp"
#include "GlideComputerInterface.hpp"
#include "InputEvents.hpp"
#include "SettingsComputer.hpp"
#include "Math/Earth.hpp"
#include "Logger/Logger.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "LocalTime.hpp"

static PeriodClock last_team_code_update;

/**
 * Constructor of the GlideComputer class
 * @return
 */
GlideComputer::GlideComputer(const Waypoints &_way_points,
                             Airspaces &_airspace_database,
                             ProtectedTaskManager &task,
                             ProtectedAirspaceWarningManager &airspace,
                             GlideComputerTaskEvents& events):
  GlideComputerAirData(_way_points, _airspace_database, airspace),
  GlideComputerTask(task),
  way_points(_way_points), protected_task_manager(task),
  TeamCodeRefId(-1)
{
  events.set_computer(*this);
  idle_clock.update();
}

/**
 * Resets the GlideComputer data
 * @param full Reset all data?
 */
void
GlideComputer::ResetFlight(const bool full)
{
  GlideComputerBlackboard::ResetFlight(full);
  GlideComputerAirData::ResetFlight(full);
  GlideComputerTask::ResetFlight(full);
  GlideComputerStats::ResetFlight(full);
}

/**
 * Initializes the GlideComputer
 */
void
GlideComputer::Initialise()
{
  GlideComputerTask::Initialise();
  ResetFlight(true);
}

/**
 * Is called by the CalculationThread and processes the received GPS data in Basic()
 */
bool
GlideComputer::ProcessGPS()
{
  PeriodClock clock;
  clock.update();

  const NMEA_INFO &basic = Basic();
  DERIVED_INFO &calculated = SetCalculated();

  calculated.local_date_time = basic.DateTime + GetUTCOffset();

  calculated.Expire(basic.Time);

  // Process basic information
  ProcessBasic();

  // Process basic task information
  ProcessBasicTask();
  ProcessMoreTask();

  // Check if everything is okay with the gps time and process it
  if (!FlightTimes()) {
    return false;
  }

  // Process extended information
  ProcessVertical();

  GlideComputerStats::ProcessClimbEvents();

  // Calculate the team code
  CalculateOwnTeamCode();

  // Calculate the bearing and range of the teammate
  CalculateTeammateBearingRange();

  // Calculate the bearing and range of the teammate
  // (if teammate is a FLARM target)
  FLARM_ScanTraffic();

  vegavoice.Update(&basic, &Calculated(), SettingsComputer());

  // update basic trace history
  if (time_advanced())
    calculated.trace_history.append(basic, Calculated());

  // Update the ConditionMonitors
  ConditionMonitorsUpdate(*this);

  calculated.time_process_gps = clock.elapsed();

  if (idle_clock.check_update(500)) {
    return true;
  } else 
    return false;
}

/**
 * Process slow calculations. Called by the CalculationThread.
 */
void
GlideComputer::ProcessIdle()
{
  PeriodClock clock;
  clock.update();

  // Log GPS fixes for internal usage
  // (snail trail, stats, olc, ...)
  DoLogging();

  GlideComputerAirData::ProcessIdle();
  GlideComputerTask::ProcessIdle();
  SetCalculated().time_process_idle = clock.elapsed();
}

bool
GlideComputer::DetermineTeamCodeRefLocation()
{
  const SETTINGS_COMPUTER &settings_computer = SettingsComputer();

  if (settings_computer.TeamCodeRefWaypoint < 0)
    return false;

  if (settings_computer.TeamCodeRefWaypoint == TeamCodeRefId)
    return TeamCodeRefFound;

  TeamCodeRefId = settings_computer.TeamCodeRefWaypoint;
  const Waypoint *wp = way_points.lookup_id(TeamCodeRefId);
  if (wp == NULL)
    return TeamCodeRefFound = false;

  TeamCodeRefLocation = wp->Location;
  return TeamCodeRefFound = true;
}

/**
 * Calculates the own TeamCode and saves it to Calculated
 */
void
GlideComputer::CalculateOwnTeamCode()
{
  // No reference waypoint for teamcode calculation chosen -> cancel
  if (!DetermineTeamCodeRefLocation())
    return;

  // Only calculate every 10sec otherwise cancel calculation
  if (!last_team_code_update.check_update(10000))
    return;

  // Get bearing and distance to the reference waypoint
  const GeoVector v = TeamCodeRefLocation.distance_bearing(Basic().Location);

  // Save teamcode to Calculated
  SetCalculated().OwnTeamCode.Update(v.Bearing, v.Distance);
}

static void
ComputeFlarmTeam(const GeoPoint &location, const GeoPoint &reference_location,
                 const FLARM_STATE &flarm, const FlarmId target_id,
                 TEAMCODE_INFO &teamcode_info)
{
  if (!flarm.available) {
    teamcode_info.flarm_teammate_code_current = false;
    return;
  }

  const FLARM_TRAFFIC *traffic = flarm.FindTraffic(target_id);
  if (traffic == NULL || !traffic->location_available) {
    teamcode_info.flarm_teammate_code_current = false;
    return;
  }

  // Set Teammate location to FLARM contact location
  teamcode_info.TeammateLocation = traffic->location;
  teamcode_info.teammate_vector = location.distance_bearing(traffic->location);
  teamcode_info.teammate_available = true;

  // Calculate distance and bearing from teammate to reference waypoint

  GeoVector v = reference_location.distance_bearing(traffic->location);

  // Calculate TeamCode and save it in Calculated
  teamcode_info.flarm_teammate_code.Update(v.Bearing, v.Distance);
  teamcode_info.flarm_teammate_code_available = true;
  teamcode_info.flarm_teammate_code_current = true;
}

static void
ComputeTeamCode(const GeoPoint &location, const GeoPoint &reference_location,
                const TeamCode &team_code,
                TEAMCODE_INFO &teamcode_info)
{
  // Calculate bearing and distance to teammate
  teamcode_info.TeammateLocation = team_code.GetLocation(reference_location);
  teamcode_info.teammate_vector =
    location.distance_bearing(teamcode_info.TeammateLocation);
  teamcode_info.teammate_available = true;
}

void
GlideComputer::CalculateTeammateBearingRange()
{
  const SETTINGS_COMPUTER &settings_computer = SettingsComputer();
  const NMEA_INFO &basic = Basic();
  TEAMCODE_INFO &teamcode_info = SetCalculated();

  // No reference waypoint for teamcode calculation chosen -> cancel
  if (!DetermineTeamCodeRefLocation())
    return;

  if (settings_computer.TeamFlarmTracking) {
    ComputeFlarmTeam(basic.Location, TeamCodeRefLocation,
                     basic.flarm, settings_computer.TeamFlarmIdTarget,
                     teamcode_info);
    CheckTeammateRange();
  } else if (settings_computer.TeammateCodeValid) {
    teamcode_info.flarm_teammate_code_available = false;

    ComputeTeamCode(basic.Location, TeamCodeRefLocation,
                    settings_computer.TeammateCode,
                    teamcode_info);
    CheckTeammateRange();
  } else {
    teamcode_info.teammate_available = false;
    teamcode_info.flarm_teammate_code_available = false;
  }
}

void
GlideComputer::CheckTeammateRange()
{
  static bool InTeamSector = false;

  // Hysteresis for GlideComputerEvent
  // If (closer than 100m to the teammates last position and "event" not reset)
  if (Calculated().teammate_vector.Distance < fixed(100) &&
      InTeamSector == false) {
    InTeamSector = true;
    // Raise GCE_TEAM_POS_REACHED event
    InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
  } else if (Calculated().teammate_vector.Distance > fixed(300)) {
    // Reset "event" when distance is greater than 300m again
    InTeamSector = false;
  }
}

void
GlideComputer::OnTakeoff()
{
  GlideComputerAirData::OnTakeoff();
  GlideComputerTask::OnTakeoff();
  InputEvents::processGlideComputer(GCE_TAKEOFF);
}

void
GlideComputer::OnLanding()
{
  GlideComputerAirData::OnLanding();
  InputEvents::processGlideComputer(GCE_LANDING);
}

void
GlideComputer::OnSwitchClimbMode(bool isclimb, bool left)
{
  GlideComputerAirData::OnSwitchClimbMode(isclimb, left);

  if (isclimb) {
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_CLIMB);
  } else {
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
  }
}

/**
 * Searches the FLARM_Traffic array for the TeamMate and updates TeamMate
 * position and TeamCode if found.
 */
void
GlideComputer::FLARM_ScanTraffic()
{
  const NMEA_INFO &basic = Basic();
  const NMEA_INFO &last_basic = LastBasic();

  if (basic.flarm.rx && last_basic.flarm.rx == 0)
    // traffic has appeared..
    InputEvents::processGlideComputer(GCE_FLARM_TRAFFIC);

  if (basic.flarm.rx == 0 && last_basic.flarm.rx)
    // traffic has disappeared..
    InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);

  if (basic.flarm.NewTraffic)
    // new traffic has appeared
    InputEvents::processGlideComputer(GCE_FLARM_NEWTRAFFIC);
}

void 
GlideComputer::OnStartTask()
{
  GlideComputerBlackboard::StartTask();
  GlideComputerStats::StartTask();

  if (logger != NULL)
    logger->LogStartEvent(Basic());
}

void 
GlideComputer::OnFinishTask()
{
  SaveFinish();

  if (logger != NULL)
    logger->LogFinishEvent(Basic());
}

void
GlideComputer::OnTransitionEnter()
{
  GlideComputerStats::SetFastLogging();
}


void
GlideComputer::set_terrain(RasterTerrain* _terrain)
{
  GlideComputerAirData::set_terrain(_terrain);
  GlideComputerTask::set_terrain(_terrain);
}
