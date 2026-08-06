// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GlideComputerBlackboard.hpp"
#include "time/PeriodClock.hpp"
#include "time/DeltaTime.hpp"
#include "GlideComputerAirData.hpp"
#include "StatsComputer.hpp"
#include "TaskComputer.hpp"
#include "LogComputer.hpp"
#include "WarningComputer.hpp"
#include "CuComputer.hpp"
#include "Engine/Contest/Solvers/Retrospective.hpp"
#include "ConditionMonitor/ConditionMonitors.hpp"
#include "ConditionMonitor/MoreConditionMonitors.hpp"

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
  WarningComputer warning_computer;
  TaskComputer task_computer;
  StatsComputer stats_computer;
  LogComputer log_computer;
  CuComputer cu_computer;

  ConditionMonitors condition_monitors;
  MoreConditionMonitors idle_condition_monitors;

  GlideComputerTaskEvents &task_events;

  const Waypoints &waypoints;

  Retrospective retrospective;
  int team_code_ref_id;
  bool team_code_ref_found;
  GeoPoint team_code_ref_location;

  PeriodClock idle_clock;

  /**
   * This object is used to check whether to update
   * DerivedInfo::trace_history.
   */
  DeltaTime trace_history_time;

public:
  GlideComputer(const ComputerSettings &_settings,
                const Waypoints &_way_points,
                Airspaces &_airspace_database,
                ProtectedTaskManager& task,
                GlideComputerTaskEvents& events);

  void SetTerrain(RasterTerrain *_terrain);

  /**
   * Silence the condition monitors, for the duration of a Resume sweep.
   *
   * Without this the six monitors fire for every replayed fix, and the pilot
   * is buried in hours of stale notifications the moment the progress dialog
   * closes.
   */
  void SetConditionMonitorsSuppressed(bool suppressed) noexcept {
    condition_monitors.SetSuppressed(suppressed);
  }

  /**
   * Stop anything reaching the pilot's IGC file, for the duration of a Resume
   * sweep.  See LogComputer::SetSuppressed().
   */
  void SetLoggingSuppressed(bool suppressed) noexcept {
    log_computer.SetSuppressed(suppressed);
  }

  /**
   * Stop announcing task starts, turnpoint advances and finishes, for the
   * duration of a Resume sweep.  See GlideComputerTaskEvents::SetSuppressed().
   */
  void SetTaskEventsSuppressed(bool suppressed) noexcept;

  void SetLogger(Logger *logger) {
    log_computer.SetLogger(logger);
  }

  /**
   * Resets the GlideComputer data
   * @param full Reset all data?
   */
  void ResetFlight(const bool full=true);

  /**
   * Initializes the GlideComputer
   */
  void Initialise();

  void Expire() {
    SetCalculated().Expire(Basic().clock);
  }

  /**
   * Is called by the CalculationThread and processes the received GPS
   * data in Basic().
   *
   * @param force forces calculation even if there was no new GPS fix
   */
  bool ProcessGPS(bool force=false); // returns true if idle needs processing

  /**
   * Process slow calculations. Called by the CalculationThread.
   */
  void ProcessIdle(bool exhaustive=false);

  /**
   * The part of ProcessIdle() that a Resume sweep is allowed to run.
   *
   * A separate entry point rather than a flag on ProcessIdle(), because a
   * flag would leave LogComputer::Run() *reachable* and rely on every future
   * edit respecting it.  If that call ever ran during a sweep, every
   * replayed fix would be written into the pilot's IGC file, duplicating the
   * whole Flight in the one artefact that matters for scoring -- and no test
   * would catch it unless it inspected the file.  An entry point that does
   * not contain the call cannot regress that way.
   *
   * Also omits the airspace warning computer, whose replayed hours of stale
   * warnings the pilot must never see.
   *
   * @param solve_contest also solve the contest, which is worth doing once at
   * the end of a sweep rather than for every replayed fix
   */
  void ProcessIdleForReplay(bool solve_contest=false);

  /**
   * Undo a Resume reconstruction that turned out to belong to a Flight that
   * has already ended.
   *
   * ResetFlight() is the only call that reaches everything a sweep rebuilt:
   * task progress lives in the task engine rather than in DerivedInfo, along
   * with the trace, contest, statistics and retrospective.  But it also wipes
   * the live takeoff that triggered the Resume in the first place, so the
   * flying state captured before the sweep is put back afterwards.
   *
   * The flying computer's own counters are reset and cannot be restored -- it
   * exposes no injection API -- but the aircraft is airborne and moving, and
   * FlyingComputer::Moving() re-arms the moving clock before the flying state
   * is re-evaluated, so the first live fix repairs that.
   *
   * @param live_flight the flying state as it was before the sweep
   */
  void RejectReconstruction(const FlyingState &live_flight);

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

  TraceComputer &GetTraceComputer() noexcept {
    return task_computer.GetTraceComputer();
  }

  const ProtectedTaskManager &GetProtectedTaskManager() const {
    return task_computer.GetProtectedTaskManager();
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

  const Retrospective &GetRetrospective() const {
    return retrospective;
  }

  void SetContestIncremental(bool incremental) {
    task_computer.SetContestIncremental(incremental);
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
  [[gnu::pure]]
  bool DetermineTeamCodeRefLocation();

  void CalculateTeammateBearingRange();

  /**
   * Calculates the own TeamCode and saves it to Calculated
   */
  void CalculateOwnTeamCode();

  void CalculateWorkingBand();
  void CalculateVarioScale();
};
