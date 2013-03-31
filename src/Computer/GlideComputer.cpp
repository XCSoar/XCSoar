/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "ComputerSettings.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "ConditionMonitor/ConditionMonitors.hpp"
#include "TeamCode.hpp"
#include "GlideComputerInterface.hpp"
#include "ComputerSettings.hpp"
#include "Logger/Logger.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

static PeriodClock last_team_code_update;

/**
 * Constructor of the GlideComputer class
 * @return
 */
GlideComputer::GlideComputer(const Waypoints &_way_points,
                             Airspaces &_airspace_database,
                             ProtectedTaskManager &task,
                             GlideComputerTaskEvents& events):
  air_data_computer(_way_points),
  task_computer(task, _airspace_database),
  warning_computer(_airspace_database),
  waypoints(_way_points),
  team_code_ref_id(-1)
{
  events.SetComputer(*this);
  idle_clock.Update();
}

/**
 * Resets the GlideComputer data
 * @param full Reset all data?
 */
void
GlideComputer::ResetFlight(const bool full)
{
  GlideComputerBlackboard::ResetFlight(full);
  air_data_computer.ResetFlight(SetCalculated(), full);
  task_computer.ResetFlight(full);
  stats_computer.ResetFlight(full);
  log_computer.Reset();

  cu_computer.Reset();
  warning_computer.Reset();
}

/**
 * Initializes the GlideComputer
 */
void
GlideComputer::Initialise()
{
  ResetFlight(true);
}

/**
 * Is called by the CalculationThread and processes the received GPS data in Basic()
 */
bool
GlideComputer::ProcessGPS(bool force)
{
  const MoreData &basic = Basic();
  DerivedInfo &calculated = SetCalculated();
  const ComputerSettings &settings = GetComputerSettings();

  const bool last_flying = calculated.flight.flying;

  calculated.date_time_local = basic.date_time_utc +
    settings.utc_offset.AsSeconds();

  calculated.Expire(basic.clock);

  // Process basic information
  air_data_computer.ProcessBasic(Basic(), SetCalculated(),
                                 GetComputerSettings());

  // Process basic task information
  task_computer.ProcessBasicTask(basic, LastBasic(),
                                 calculated,
                                 GetComputerSettings(),
                                 force);
  task_computer.ProcessMoreTask(basic, calculated, GetComputerSettings());

  // Check if everything is okay with the gps time and process it
  if (!air_data_computer.FlightTimes(Basic(), LastBasic(), SetCalculated(),
                                     GetComputerSettings()))
    return false;

  TakeoffLanding(last_flying);

  if (!time_retreated())
    task_computer.ProcessAutoTask(basic, calculated);

  // Process extended information
  air_data_computer.ProcessVertical(Basic(), LastBasic(),
                                    SetCalculated(),
                                    GetComputerSettings());

  if (!time_retreated())
    stats_computer.ProcessClimbEvents(calculated);

  // Calculate the team code
  CalculateOwnTeamCode();

  // Calculate the bearing and range of the teammate
  CalculateTeammateBearingRange();

  vegavoice.Update(basic, Calculated(), GetComputerSettings().voice);

  // update basic trace history
  if (time_advanced())
    calculated.trace_history.append(basic);

  // Update the ConditionMonitors
  ConditionMonitorsUpdate(Basic(), Calculated(), settings);

  return idle_clock.CheckUpdate(500);
}

/**
 * Process slow calculations. Called by the CalculationThread.
 */
void
GlideComputer::ProcessIdle(bool exhaustive)
{
  // Log GPS fixes for internal usage
  // (snail trail, stats, olc, ...)
  stats_computer.DoLogging(Basic(), Calculated());
  log_computer.Run(Basic(), Calculated(), GetComputerSettings().logger);

  task_computer.ProcessIdle(Basic(), SetCalculated(), GetComputerSettings(),
                            exhaustive);

  if (time_advanced())
    warning_computer.Update(GetComputerSettings(), Basic(), LastBasic(),
                            Calculated(), SetCalculated().airspace_warnings);
}

bool
GlideComputer::DetermineTeamCodeRefLocation()
{
  const TeamCodeSettings &settings = GetComputerSettings().team_code;

  if (settings.team_code_reference_waypoint < 0)
    return false;

  if (settings.team_code_reference_waypoint == team_code_ref_id)
    return team_code_ref_found;

  team_code_ref_id = settings.team_code_reference_waypoint;
  const Waypoint *wp = waypoints.LookupId(team_code_ref_id);
  if (wp == NULL)
    return team_code_ref_found = false;

  team_code_ref_location = wp->location;
  return team_code_ref_found = true;
}

/**
 * Calculates the own TeamCode and saves it to Calculated
 */
inline void
GlideComputer::CalculateOwnTeamCode()
{
  // No reference waypoint for teamcode calculation chosen -> cancel
  if (!DetermineTeamCodeRefLocation())
    return;

  // Only calculate every 10sec otherwise cancel calculation
  if (!last_team_code_update.CheckUpdate(10000))
    return;

  // Get bearing and distance to the reference waypoint
  const GeoVector v = team_code_ref_location.DistanceBearing(Basic().location);

  // Save teamcode to Calculated
  SetCalculated().own_teammate_code.Update(v.bearing, v.distance);
}

static void
ComputeFlarmTeam(const GeoPoint &location, const GeoPoint &reference_location,
                 const TrafficList &traffic_list, const FlarmId target_id,
                 TeamInfo &teamcode_info)
{
  const FlarmTraffic *traffic = traffic_list.FindTraffic(target_id);
  if (traffic == NULL || !traffic->location_available) {
    teamcode_info.flarm_teammate_code_current = false;
    return;
  }

  // Set Teammate location to FLARM contact location
  teamcode_info.teammate_location = traffic->location;
  teamcode_info.teammate_vector = location.DistanceBearing(traffic->location);
  teamcode_info.teammate_available = true;

  // Calculate distance and bearing from teammate to reference waypoint

  GeoVector v = reference_location.DistanceBearing(traffic->location);

  // Calculate TeamCode and save it in Calculated
  teamcode_info.flarm_teammate_code.Update(v.bearing, v.distance);
  teamcode_info.flarm_teammate_code_current = true;
}

static void
ComputeTeamCode(const GeoPoint &location, const GeoPoint &reference_location,
                const TeamCode &team_code,
                TeamInfo &teamcode_info)
{
  // Calculate bearing and distance to teammate
  teamcode_info.teammate_location = team_code.GetLocation(reference_location);
  teamcode_info.teammate_vector =
    location.DistanceBearing(teamcode_info.teammate_location);
  teamcode_info.teammate_available = true;
}

void
GlideComputer::CalculateTeammateBearingRange()
{
  const TeamCodeSettings &settings = GetComputerSettings().team_code;
  const NMEAInfo &basic = Basic();
  TeamInfo &teamcode_info = SetCalculated();

  // No reference waypoint for teamcode calculation chosen -> cancel
  if (!DetermineTeamCodeRefLocation())
    return;

  if (settings.team_flarm_id.IsDefined()) {
    ComputeFlarmTeam(basic.location, team_code_ref_location,
                     basic.flarm.traffic, settings.team_flarm_id,
                     teamcode_info);
  } else if (settings.team_code.IsDefined()) {
    teamcode_info.flarm_teammate_code.Clear();

    ComputeTeamCode(basic.location, team_code_ref_location,
                    settings.team_code,
                    teamcode_info);
  } else {
    teamcode_info.teammate_available = false;
    teamcode_info.flarm_teammate_code.Clear();
  }
}

inline void
GlideComputer::OnTakeoff()
{
  // reset stats on takeoff
  air_data_computer.ResetFlight(SetCalculated(), false);

  // save stats in case we never finish
  SaveFinish();
}

inline void
GlideComputer::OnLanding()
{
  // JMWX  restore data calculated at finish so
  // user can review flight as at finish line

  if (Calculated().ordered_task_stats.task_finished)
    RestoreFinish();
}

inline void
GlideComputer::TakeoffLanding(bool last_flying)
{
  if (Calculated().flight.flying && !last_flying)
    OnTakeoff();
  else if (!Calculated().flight.flying && last_flying)
    OnLanding();
}

void 
GlideComputer::OnStartTask()
{
  GlideComputerBlackboard::StartTask();
  air_data_computer.ResetStats();
  stats_computer.StartTask(Basic());
  log_computer.StartTask(Basic());
}

void 
GlideComputer::OnFinishTask()
{
  SaveFinish();
}

void
GlideComputer::OnTransitionEnter()
{
  log_computer.SetFastLogging();
}


void
GlideComputer::SetTerrain(RasterTerrain* _terrain)
{
  air_data_computer.SetTerrain(_terrain);
  task_computer.SetTerrain(_terrain);
}
