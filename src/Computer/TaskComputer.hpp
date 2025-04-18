// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  void LockedCopyTraceTo(TracePointVector &v,
                         std::chrono::duration<unsigned> min_time,
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
