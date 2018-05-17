/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_GLIDECOMPUTER_TASK_HPP
#define XCSOAR_GLIDECOMPUTER_TASK_HPP

#include "RouteComputer.hpp"
#include "TraceComputer.hpp"
#include "ContestComputer.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "NMEA/Validity.hpp"

struct NMEAInfo;
class ProtectedTaskManager;
class ProtectedAirspaceWarningManager;

class TaskComputer
{
  ProtectedTaskManager &task;

  RouteComputer route;

  TraceComputer trace;

  ContestComputer contest;

  AircraftState last_state;
  bool valid_last_state;

  bool last_flying;

  Validity last_location_available;

public:
  TaskComputer(ProtectedTaskManager &_task,
               const Airspaces &airspace_database,
               const ProtectedAirspaceWarningManager *warnings);

  const ProtectedTaskManager &GetProtectedTaskManager() const {
    return task;
  }

  const ProtectedRoutePlanner &GetProtectedRoutePlanner() const {
    return route.GetProtectedRoutePlanner();
  }

  void ClearAirspaces() {
    route.ClearAirspaces();
  }

  const TraceComputer &GetTraceComputer() const {
    return trace;
  }

  void LockedCopyTraceTo(TracePointVector &v) const {
    trace.LockedCopyTo(v);
  }

  void LockedCopyTraceTo(TracePointVector &v, unsigned min_time,
                         const GeoPoint &location, double resolution) const {
    trace.LockedCopyTo(v, min_time, location, resolution);
  }

  void ProcessBasicTask(const MoreData &basic,
                        DerivedInfo &calculated,
                        const ComputerSettings &settings_computer,
                        bool force);
  void ProcessMoreTask(const MoreData &basic, DerivedInfo &calculated,
                       const ComputerSettings &settings_computer);

  void ResetFlight(const bool full=true);

  void SetTerrain(const RasterTerrain* _terrain);

  void SetContestIncremental(bool incremental) {
    contest.SetIncremental(incremental);
  }

  /**
   * Auto-create a task on takeoff that leads back home.
   */
  void ProcessAutoTask(const NMEAInfo &basic, const DerivedInfo &calculated);

  void ProcessIdle(const MoreData &basic, DerivedInfo &calculated,
                   const ComputerSettings &settings_computer,
                   bool exhaustive=false);
};

#endif
