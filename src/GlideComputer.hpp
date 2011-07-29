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

#if !defined(XCSOAR_GLIDECOMPUTER_HPP)
#define XCSOAR_GLIDECOMPUTER_HPP

#include "Audio/VegaVoice.hpp"
#include "GPSClock.hpp"
#include "PeriodClock.hpp"
#include "GlideComputerAirData.hpp"
#include "GlideComputerStats.hpp"
#include "GlideComputerTask.hpp"
#include "Compiler.h"

class Waypoints;
class ProtectedTaskManager;
class ProtectedRoutePlanner;
class RoutePlannerGlue;
class GlideComputerTaskEvents;
class RasterTerrain;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputer:
    public GlideComputerAirData, public GlideComputerTask,
    public GlideComputerStats
{
  const Waypoints &way_points;
  ProtectedTaskManager &protected_task_manager;

  int TeamCodeRefId;
  bool TeamCodeRefFound;
  GeoPoint TeamCodeRefLocation;

  PeriodClock idle_clock;
  VegaVoice vegavoice;

public:
  GlideComputer(const Waypoints &_way_points,
                Airspaces &_airspace_database,
                ProtectedTaskManager& task,
                ProtectedRoutePlanner &protected_route_planner,
                const RoutePlannerGlue &route_planner,
                ProtectedAirspaceWarningManager &_awm,
                GlideComputerTaskEvents& events);
  virtual ~GlideComputer() {}

  void set_terrain(RasterTerrain* _terrain);

  void ResetFlight(const bool full=true);
  void Initialise();

  void Expire() {
    SetCalculated().Expire(Basic().clock);
  }

  bool ProcessGPS(); // returns true if idle needs processing
  void ProcessIdle();

  void OnStartTask();
  void OnFinishTask();
  void OnTransitionEnter();

protected:
  virtual void OnTakeoff();
  virtual void OnLanding();
  virtual void OnSwitchClimbMode(bool isclimb, bool left);

private:

  /**
   * Fill the cache variable TeamCodeRefLocation.
   *
   * @return true if the location was found, false if the
   * TeamCodeRefLocation variable is undefined
   */
  gcc_pure
  bool DetermineTeamCodeRefLocation();

  void CheckTeammateRange();
  void CalculateTeammateBearingRange();
  void CalculateOwnTeamCode();
  void FLARM_ScanTraffic();
};

#endif
