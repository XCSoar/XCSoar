/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "McReady.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Persist.hpp"
#include "ConditionMonitor.hpp"
#include "TeamCodeCalculation.h"
#include "Components.hpp"
#ifdef OLD_TASK
#include "WayPointList.hpp"
#include "Task.h"
#endif

/**
 * Constructor of the GlideComputer class
 * @return
 */
GlideComputer::GlideComputer()
{

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

void
GlideComputer::StartTask(const bool do_advance, const bool do_announce)
{
  //  GlideComputerBlackboard::StartTask();
  GlideComputerStats::StartTask();

  if (do_announce) {
    AnnounceWayPointSwitch(do_advance);
  } else {
    GlideComputerTask::StartTask(do_advance, do_announce);
  }
}

/**
 * Initializes the GlideComputer
 */
void
GlideComputer::Initialise()
{
  GlideComputerBlackboard::Initialise();
  GlideComputerAirData::Initialise();
  GlideComputerTask::Initialise();
  GlideComputerStats::Initialise();
  ResetFlight(true);

  LoadCalculationsPersist(&SetCalculated());
  DeleteCalculationsPersist();
  // required to allow fail-safe operation
  // if the persistent file is corrupt and causes a crash

  ResetFlight(false);
}

/**
 * Log GPS fixes for GlideComputerStats and
 * GlideComputerTask, if valid fix is detected
 */
void
GlideComputer::DoLogging()
{
  // call Stats::DoLogging()
  // -> returns if valid fix
  // if (valid fix)
  if (GlideComputerStats::DoLogging()) {
    // call Task::DoLogging()
    GlideComputerTask::DoLogging();
  }
}

/**
 * Is called by the CalculationThread and processes the received GPS data in Basic()
 */
bool
GlideComputer::ProcessGPS()
{
  double mc = GlidePolar::GetMacCready();
  double ce = GlidePolar::GetCruiseEfficiency();

  // Process basic information
  ProcessBasic();

  // Process basic task information
  ProcessBasicTask(mc, ce);

  // Check if everything is okay with the gps time and process it
  if (!FlightTimes()) {
    return false;
  }

  // Process extended information
  ProcessVertical();

  // Calculate the team code
  CalculateOwnTeamCode();

  SetCalculated().TeammateCodeValid = SettingsComputer().TeammateCodeValid;

  // Calculate the bearing and range of the teammate
  CalculateTeammateBearingRange();

  // Calculate the bearing and range of the teammate
  // (if teammate is a FLARM target)
  FLARM_ScanTraffic();

  vegavoice.Update(&Basic(), &Calculated(), SettingsComputer());

  // Update the ConditionMonitors
  ConditionMonitorsUpdate(*this);

  return true;
}

/**
 * Calls GlideComputerAirData::ProcessVario()
 */
bool
GlideComputer::ProcessVario()
{
  return GlideComputerAirData::ProcessVario();
}

/**
 * Calls GlideComputerStats::SaveTaskSpeed(val)
 * @param val Task speed
 */
void
GlideComputer::SaveTaskSpeed(double val)
{
  GlideComputerStats::SaveTaskSpeed(val);
}

/**
 * Process slow calculations. Called by the CalculationThread.
 */
void
GlideComputer::ProcessIdle()
{
  /*
  // VENTA3 Alternates
  if ( EnableAlternate1 == true ) DoAlternates(Basic, Calculated,Alternate1);
  if ( EnableAlternate2 == true ) DoAlternates(Basic, Calculated,Alternate2);
  if ( EnableBestAlternate == true ) DoAlternates(Basic, Calculated,BestAlternate);
  */

  // Log GPS fixes for internal usage
  // (snail trail, stats, olc, ...)
  DoLogging();

  CalculateWaypointReachable();

#ifdef OLD_TASK
  // if (Task is not aborted and Task consists of more than one waypoint)
  if (!task.TaskIsTemporary()) {
    double mc = GlidePolar::GetMacCready();
    InSector();
    DoAutoMacCready(mc);
    IterateEffectiveMacCready();
  }
#endif

  GlideComputerAirData::ProcessIdle();
}

bool
GlideComputer::InsideStartHeight(const DWORD Margin) const
{
  return GlideComputerTask::InsideStartHeight(Margin);
}

bool
GlideComputer::ValidStartSpeed(const DWORD Margin) const
{
  return GlideComputerTask::ValidStartSpeed(Margin);
}

void
GlideComputer::IterateEffectiveMacCready()
{

}

void
GlideComputer::SetLegStart()
{
    GlideComputerTask::SetLegStart();
    GlideComputerStats::SetLegStart();
}

#include "Math/NavFunctions.hpp" // used for team code
#include "InputEvents.h"
#include "SettingsComputer.hpp"
#include "PeriodClock.hpp"
#include "Math/Earth.hpp"

static PeriodClock last_team_code_update;

/**
 * Calculates the own TeamCode and saves it to Calculated
 */
void
GlideComputer::CalculateOwnTeamCode()
{
#ifdef OLD_TASK
  // No reference waypoint for teamcode calculation chosen -> cancel
  if (SettingsComputer().TeamCodeRefWaypoint < 0)
    return;

  // Only calculate every 10sec otherwise cancel calculation
  if (!last_team_code_update.check_update(10000))
    return;

  // JMW TODO: locking
  double distance = 0;
  double bearing = 0;
  TCHAR code[10];

  // Get bearing and distance to the reference waypoint
  LL_to_BearRange(
      way_points.get(SettingsComputer().TeamCodeRefWaypoint).Location.Latitude,
      way_points.get(SettingsComputer().TeamCodeRefWaypoint).Location.Longitude,
      Basic().Location.Latitude,
      Basic().Location.Longitude,
      &bearing, &distance);

  // Calculate teamcode from bearing and distance
  GetTeamCode(code, bearing, distance);

  // QUESTION TB: why save the own bearing/distance as TeammateBearing/Range ??
  SetCalculated().TeammateBearing = bearing;
  SetCalculated().TeammateRange = distance;

  // Save teamcode to Calculated
  _tcsncpy(SetCalculated().OwnTeamCode, code, 5);
#else
  return;
#endif
}

void
GlideComputer::CalculateTeammateBearingRange()
{
#ifdef OLD_TASK
  static bool InTeamSector = false;

  // No reference waypoint for teamcode calculation chosen -> cancel
  if (SettingsComputer().TeamCodeRefWaypoint < 0)
    return;

  double ownDistance = 0;
  double ownBearing = 0;
  double mateDistance = 0;
  double mateBearing = 0;

  // Get own bearing and distance to the reference waypoint
  LL_to_BearRange(
      way_points.get(SettingsComputer().TeamCodeRefWaypoint).Location.Latitude,
      way_points.get(SettingsComputer().TeamCodeRefWaypoint).Location.Longitude,
      Basic().Location.Latitude,
      Basic().Location.Longitude,
      &ownBearing, &ownDistance);

  // If (TeamCode exists and is valid)
  if (SettingsComputer().TeammateCodeValid) {
    // Calculate bearing and distance to teammate
    CalcTeammateBearingRange(ownBearing, ownDistance,
        Calculated().TeammateCode, &mateBearing, &mateDistance);

    // TODO code ....change the result of CalcTeammateBearingRange to do this !
    if (mateBearing > 180) {
      mateBearing -= 180;
    } else {
      mateBearing += 180;
    }

    // Save bearing and distance to teammate in Calculated
    SetCalculated().TeammateBearing = mateBearing;
    SetCalculated().TeammateRange = mateDistance;

    // Calculate GPS position of the teammate and save it in Calculated
    FindLatitudeLongitude(Basic().Location, mateBearing, mateDistance,
        &SetCalculated().TeammateLocation);

    // Hysteresis for GlideComputerEvent
    // If (closer than 100m to the teammates last position and "event" not reset)
    if (mateDistance < 100 && InTeamSector == false) {
      InTeamSector = true;
      // Raise GCE_TEAM_POS_REACHED event
      InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
    } else if (mateDistance > 300) {
      // Reset "event" when distance is greater than 300m again
      InTeamSector = false;
    }
  } else {
    SetCalculated().TeammateBearing = 0;
    SetCalculated().TeammateRange = 0;
  }
#else
  return;
#endif
}

void
GlideComputer::OnTakeoff()
{
  GlideComputerAirData::OnTakeoff();
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
#ifdef OLD_TASK
  // If (not FLARM available) cancel
  if (!Basic().FLARM_Available)
    return;

  // Iterate through all FLARM contacts
  for (int flarm_slot = 0; flarm_slot < FLARM_MAX_TRAFFIC; flarm_slot++) {
    // If (FLARM contact found)
    if (Basic().FLARM_Traffic[flarm_slot].ID > 0) {
      // JMW TODO: this is dangerous, it uses the task!
      // it should be done outside the parser/comms thread

      // If (FLARM contact == TeamMate)
      if ((Basic().FLARM_Traffic[flarm_slot].ID == SettingsComputer().TeamFlarmIdTarget)
          && way_points.verify_index(SettingsComputer().TeamCodeRefWaypoint)) {
        double bearing;
        double distance;

        // Set Teammate location to FLARM contact location
        SetCalculated().TeammateLocation
            = Basic().FLARM_Traffic[flarm_slot].Location;

        // Calculate distance and bearing from teammate to reference waypoint
        DistanceBearing(
            way_points.get(SettingsComputer().TeamCodeRefWaypoint).Location,
            Basic().FLARM_Traffic[flarm_slot].Location, &distance, &bearing);

        // Calculate TeamCode and save it in Calculated
        GetTeamCode(SetCalculated().TeammateCode, bearing, distance);
        SetCalculated().TeammateCodeValid = true;
      }
    }
  }
#endif
}
