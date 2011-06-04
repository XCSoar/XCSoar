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
#include "Protection.hpp"
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
#include "Interface.hpp"
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

  SetCalculated().local_date_time = Basic().DateTime + GetUTCOffset();

  SetCalculated().expire(Basic().Time);

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

  // Calculate the team code
  CalculateOwnTeamCode();

  // Calculate the bearing and range of the teammate
  CalculateTeammateBearingRange();

  // Calculate the bearing and range of the teammate
  // (if teammate is a FLARM target)
  FLARM_ScanTraffic();

  vegavoice.Update(&Basic(), &Calculated(), SettingsComputer());

  // update basic trace history
  if (time_advanced())
    SetCalculated().trace_history.append(Basic(), Calculated());

  // Update the ConditionMonitors
  ConditionMonitorsUpdate(*this);

  SetCalculated().time_process_gps = clock.elapsed();

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
  if (SettingsComputer().TeamCodeRefWaypoint < 0)
    return false;

  if (SettingsComputer().TeamCodeRefWaypoint == TeamCodeRefId)
    return TeamCodeRefFound;

  TeamCodeRefId = SettingsComputer().TeamCodeRefWaypoint;
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
  Angle bearing;
  fixed distance;
  TeamCodeRefLocation.distance_bearing(Basic().Location, distance, bearing);

  // Save teamcode to Calculated
  SetCalculated().OwnTeamCode.Update(bearing, distance);
}

void
GlideComputer::CalculateTeammateBearingRange()
{
  // No reference waypoint for teamcode calculation chosen -> cancel
  if (!DetermineTeamCodeRefLocation())
    return;

  if (Basic().flarm.available && SettingsComputer().TeamFlarmTracking) {
    const FLARM_TRAFFIC *traffic =
        Basic().flarm.FindTraffic(SettingsComputer().TeamFlarmIdTarget);

    if (traffic && traffic->location_available) {
      // Set Teammate location to FLARM contact location
      SetCalculated().TeammateLocation = traffic->location;
      Basic().Location.distance_bearing(traffic->location,
                                        SetCalculated().TeammateRange,
                                        SetCalculated().TeammateBearing);

      // Calculate distance and bearing from teammate to reference waypoint

      Angle bearing;
      fixed distance;
      TeamCodeRefLocation.distance_bearing(traffic->location,
                                           distance, bearing);

      // Calculate TeamCode and save it in Calculated
      XCSoarInterface::SetSettingsComputer().TeammateCode.Update(bearing, distance);
      XCSoarInterface::SetSettingsComputer().TeammateCodeValid = true;

      CheckTeammateRange();

      return;
    }
  }

  // If (TeamCode exists and is valid)
  if (SettingsComputer().TeammateCodeValid) {
    // Calculate bearing and distance to teammate
    SetCalculated().TeammateLocation =
        SettingsComputer().TeammateCode.GetLocation(TeamCodeRefLocation);

    GeoVector team_vector(Basic().Location, Calculated().TeammateLocation);

    // Save bearing and distance to teammate in Calculated
    SetCalculated().TeammateBearing = team_vector.Bearing;
    SetCalculated().TeammateRange = team_vector.Distance;

    CheckTeammateRange();
  } else {
    SetCalculated().TeammateBearing = Angle::degrees(fixed_zero);
    SetCalculated().TeammateRange = fixed_zero;
  }
}

void
GlideComputer::CheckTeammateRange()
{
  static bool InTeamSector = false;

  // Hysteresis for GlideComputerEvent
  // If (closer than 100m to the teammates last position and "event" not reset)
  if (Calculated().TeammateRange < fixed(100) && InTeamSector == false) {
    InTeamSector = true;
    // Raise GCE_TEAM_POS_REACHED event
    InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
  } else if (Calculated().TeammateRange > fixed(300)) {
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

void
GlideComputer::OnDepartedThermal()
{
  GlideComputerAirData::OnDepartedThermal();
  GlideComputerStats::OnDepartedThermal();
}

/**
 * Searches the FLARM_Traffic array for the TeamMate and updates TeamMate
 * position and TeamCode if found.
 */
void
GlideComputer::FLARM_ScanTraffic()
{
  if (Basic().flarm.rx && LastBasic().flarm.rx == 0)
    // traffic has appeared..
    InputEvents::processGlideComputer(GCE_FLARM_TRAFFIC);

  if (Basic().flarm.rx == 0 && LastBasic().flarm.rx)
    // traffic has disappeared..
    InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);

  if (Basic().flarm.NewTraffic)
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
