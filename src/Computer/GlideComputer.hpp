/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "GlideComputerBlackboard.hpp"
#include "Audio/VegaVoice.hpp"
#include "Time/PeriodClock.hpp"
#include "GlideComputerAirData.hpp"
#include "GlideComputerStats.hpp"
#include "GlideComputerTask.hpp"
#include "LogComputer.hpp"
#include "WarningComputer.hpp"
#include "CuComputer.hpp"
#include "Compiler.h"

class Waypoints;
class ProtectedTaskManager;
class GlideComputerTaskEvents;
class RasterTerrain;

// TODO: replace copy constructors so copies of these structures
// do not replicate the large items or items that should be singletons
// OR: just make them static?

class GlideComputer : public GlideComputerBlackboard
{
  GlideComputerAirData air_data_computer;
  GlideComputerTask task_computer;
  GlideComputerStats stats_computer;
  LogComputer log_computer;
  CuComputer cu_computer;
  WarningComputer warning_computer;

  const Waypoints &waypoints;

  int team_code_ref_id;
  bool team_code_ref_found;
  GeoPoint team_code_ref_location;

  PeriodClock idle_clock;
  VegaVoice vegavoice;

public:
  GlideComputer(const Waypoints &_way_points,
                Airspaces &_airspace_database,
                ProtectedTaskManager& task,
                GlideComputerTaskEvents& events);
  virtual ~GlideComputer() {}

  void SetTerrain(RasterTerrain *_terrain);

  void SetLogger(Logger *logger) {
    log_computer.SetLogger(logger);
  }

  void ResetFlight(const bool full=true);
  void Initialise();

  void Expire() {
    SetCalculated().Expire(Basic().clock);
  }

  /**
   * @param force forces calculation even if there was no new GPS fix
   */
  bool ProcessGPS(bool force=false); // returns true if idle needs processing

  void ProcessIdle(bool exhaustive=false);

  void ProcessExhaustive() {
    ProcessIdle(true);
  }

  void OnStartTask();
  void OnFinishTask();
  void OnTransitionEnter();

  const WindStore &GetWindStore() const {
    return air_data_computer.GetWindStore();
  }

  const CuSonde &GetCuSonde() const {
    return cu_computer.GetCuSonde();
  }

  ProtectedAirspaceWarningManager &GetAirspaceWarnings() {
    return warning_computer.GetManager();
  }

  const ProtectedAirspaceWarningManager &GetAirspaceWarnings() const {
    return warning_computer.GetManager();
  }

  const TraceComputer &GetTraceComputer() const {
    return task_computer.GetTraceComputer();
  }

  const ProtectedRoutePlanner &GetProtectedRoutePlanner() const {
    return task_computer.GetProtectedRoutePlanner();
  }

  void ClearAirspaces() {
    task_computer.ClearAirspaces();
  }

  const FlightStatistics &GetFlightStats() const {
    return stats_computer.GetFlightStats();
  }

protected:
  void OnTakeoff();
  void OnLanding();

  /**
   * Detects takeoff and landing events
   */
  void TakeoffLanding(bool last_flying);

private:

  /**
   * Fill the cache variable TeamCodeRefLocation.
   *
   * @return true if the location was found, false if the
   * TeamCodeRefLocation variable is undefined
   */
  gcc_pure
  bool DetermineTeamCodeRefLocation();

  void CalculateTeammateBearingRange();
  void CalculateOwnTeamCode();
};

#endif
