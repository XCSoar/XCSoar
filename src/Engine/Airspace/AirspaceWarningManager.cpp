// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningManager.hpp"
#include "Geo/GeoVector.hpp"
#include "Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceIntersectionVisitor.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "Task/Stats/TaskStats.hpp"
#include "util/PrintException.hxx"
#include "LogFile.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>

static constexpr double CRUISE_FILTER_FACT = 0.5;

/**
 * One line when a warning starts or becomes more urgent. The
 * aircraft position is where the alert was raised; the intercept is
 * the predicted entry or exit point.
 */
static void
LogAirspaceWarning(const AirspaceWarning &warning,
                   const AircraftState &state) noexcept
{
  const char *kind = warning.IsInside() ? "inside" : "near";
  const GeoPoint &here = state.location;
  const AirspaceInterceptSolution &solution = warning.GetSolution();
  const char *name = warning.GetAirspace().GetName();

  if (solution.IsValid() && solution.location.IsValid())
    LogFmt("Airspace warning: {} \"{}\" at {:.5f},{:.5f}"
           " intercept {:.5f},{:.5f}",
           kind, name,
           here.latitude.Degrees(), here.longitude.Degrees(),
           solution.location.latitude.Degrees(),
           solution.location.longitude.Degrees());
  else
    LogFmt("Airspace warning: {} \"{}\" at {:.5f},{:.5f}",
           kind, name,
           here.latitude.Degrees(), here.longitude.Degrees());
}

/**
 * Stable id for NOTAM day-ack: short identifier in #GetStationName().
 */
[[gnu::pure]]
static const char *
NotamDayAckKey(const AbstractAirspace &airspace) noexcept
{
  if (airspace.GetType() != AirspaceClass::NOTAM)
    return nullptr;
  const char *const s = airspace.GetStationName();
  return (s != nullptr && s[0] != '\0') ? s : nullptr;
}

void
AirspaceWarningManager::ApplyNotamDayState(AirspaceWarning &warning) noexcept
{
  if (const char *key = NotamDayAckKey(warning.GetAirspace())) {
    if (notam_day_ack_by_station.contains(key))
      warning.AcknowledgeDay(true);
    if (notam_day_cleared_by_station.contains(key))
      warning.SetCleared(true);
  }
}

void
AirspaceWarningManager::ApplyRememberedDayState(AirspaceWarning &warning) noexcept
{
  ApplyNotamDayState(warning);
  if (ack_day_airspaces.contains(&warning.GetAirspace()))
    warning.AcknowledgeDay(true);
}

void
AirspaceWarningManager::DropAckDayForAbsentAirspaces() noexcept
{
  if (ack_day_airspaces.empty())
    return;

  std::unordered_set<const AbstractAirspace *> present;
  for (const auto &airspace : airspaces.QueryAll())
    present.insert(&airspace.GetAirspace());

  for (auto it = ack_day_airspaces.begin(); it != ack_day_airspaces.end();) {
    if (present.contains(*it))
      ++it;
    else
      it = ack_day_airspaces.erase(it);
  }
}

AirspaceWarningManager::AirspaceWarningManager(const AirspaceWarningConfig &_config,
                                               const Airspaces &_airspaces)
  :airspaces(_airspaces)
{
  /* force filter initialisation in the first SetConfig() call */
  config.warning_time = AirspaceWarningConfig::Duration::max();

  SetConfig(_config);
}

const FlatProjection &
AirspaceWarningManager::GetProjection() const
{
  return airspaces.GetProjection();
}

void
AirspaceWarningManager::SetConfig(const AirspaceWarningConfig &_config)
{
  const bool modified_warning_time =
    _config.warning_time != config.warning_time;

  config = _config;

  if (modified_warning_time) {
    SetPredictionTimeGlide(config.warning_time);
    SetPredictionTimeFilter(config.warning_time);
  }
}

void
AirspaceWarningManager::Reset(const AircraftState &state)
{
  ++serial;
  /* File airspaces are still in the database. NOTAM objects that the
     refresh already removed are not, and their addresses must not
     outlive them. */
  DropAckDayForAbsentAirspaces();
  ack_day_airspace_serial = airspaces.GetSerial();
  warnings.clear();
  /* Ack Day outlives a time jump. clear() drops it with the database. */
  notam_day_cleared_by_station.clear();
  cruise_filter.Reset(state);
  circling_filter.Reset(state);
}

void 
AirspaceWarningManager::SetPredictionTimeGlide(FloatDuration time) noexcept
{
  prediction_time_glide = time;
}

void 
AirspaceWarningManager::SetPredictionTimeFilter(FloatDuration time) noexcept
{
  prediction_time_filter = time;
  cruise_filter.Design(std::max(prediction_time_filter * CRUISE_FILTER_FACT,
                                FloatDuration{10}));
  circling_filter.Design(std::max(prediction_time_filter,
                                  FloatDuration{10}));
}

AirspaceWarning& 
AirspaceWarningManager::GetWarning(ConstAirspacePtr airspace)
{
  AirspaceWarning* warning = GetWarningPtr(*airspace);
  if (warning)
    return *warning;

  // not found, create new entry
  ++serial;
  warnings.emplace_back(std::move(airspace));
  ApplyRememberedDayState(warnings.back());
  return warnings.back();
}


AirspaceWarning *
AirspaceWarningManager::GetWarningPtr(const AbstractAirspace &airspace) noexcept
{
  for (auto &w : warnings)
    if (&(w.GetAirspace()) == &airspace)
      return &w;

  return nullptr;
}

AirspaceWarning *
AirspaceWarningManager::FindWarningByNotamDayAckKey(
    const std::string_view key) noexcept
{
  for (auto &warning : warnings) {
    const char *const warning_key = NotamDayAckKey(warning.GetAirspace());
    if (warning_key != nullptr && key == warning_key)
      return &warning;
  }

  return nullptr;
}

const AirspaceWarning *
AirspaceWarningManager::FindWarningByNotamDayAckKey(
    const std::string_view key) const noexcept
{
  return const_cast<AirspaceWarningManager *>(this)
    ->FindWarningByNotamDayAckKey(key);
}

AirspaceWarning *
AirspaceWarningManager::GetNewWarningPtr(ConstAirspacePtr airspace)
{
  ++serial;
  warnings.emplace_back(airspace);
  ApplyRememberedDayState(warnings.back());
  return &warnings.back();
}

bool 
AirspaceWarningManager::Update(const AircraftState& state,
                               const GlidePolar &glide_polar,
                               const TaskStats &task_stats,
                               const bool circling,
                               const std::chrono::duration<unsigned> dt)
{
  last_location = state.location;
  last_altitude = state.altitude;
  last_altitude_agl = state.altitude_agl;
  last_vario = state.vario;

  bool changed = false;

  // update warning states
  if (airspaces.IsEmpty()) {
    /* Drop stale warnings and Ack Day pointers. A new airspace can
       reuse an old address. Station-keyed NOTAM Ack Day stays. */
    if (warnings.empty() && ack_day_airspaces.empty())
      return false;

    ++serial;
    warnings.clear();
    ack_day_airspaces.clear();
    ack_day_airspace_serial = airspaces.GetSerial();
    return true;
  }

  if (airspaces.GetSerial() != ack_day_airspace_serial) {
    DropAckDayForAbsentAirspaces();
    ack_day_airspace_serial = airspaces.GetSerial();
  }

  // save old state
  for (auto &w : warnings)
    w.SaveState();

  // check from strongest to weakest alerts
  UpdateInside(state, glide_polar);
  UpdateGlide(state, glide_polar);
  UpdateFilter(state, circling);
  UpdateTask(state, glide_polar, task_stats);

  // apply clearance suppression / exit-warning generation
  ProcessClearanceIntervals(state, glide_polar, circling, task_stats);

  // action changes
  for (auto it = warnings.begin(), end = warnings.end(); it != end;) {
    if (it->WarningLive(config.acknowledgement_time, dt)) {
      if (it->ChangedState()) {
        changed = true;
        if (it->IsActive())
          LogAirspaceWarning(*it, state);
      }

      it++;
    } else {
      it = warnings.erase(it);
      changed = true;
    }
  }

  // sort by importance, most severe top
  warnings.sort();

  if (changed)
    ++serial;

  return changed;
}

/**
 * Class used temporarily to check intersections with warning system
 */
class AirspaceIntersectionWarningVisitor final
  : public AirspaceIntersectionVisitor
{
  const AircraftState state;
  const AirspaceAircraftPerformance &perf;
  AirspaceWarningManager &warning_manager;
  const AirspaceWarning::State warning_state;
  const FloatDuration max_time;
  const GeoPoint location_predicted;
  const FlatProjection &projection;
  bool found = false;
  const double max_alt;
  bool mode_inside = false;
  PathAltitudeProfile path;

public:
  /**
   * Constructor
   *
   * @param state State of aircraft
   * @param perf Aircraft performance model
   * @param warning_manager Warning manager to add items to
   * @param warning_state Type of warning
   * @param max_time Time limit of intercept
   * @param location_predicted Location of intercept
   * @param projection Projected location of intercept
   * @param max_alt Maximum height of base to allow (optional)
   *
   * @return Initialised object
   */
  AirspaceIntersectionWarningVisitor(
      const AircraftState &_state,
      const AirspaceAircraftPerformance &_perf,
      AirspaceWarningManager &_warning_manager,
      const AirspaceWarning::State _warning_state,
      const FloatDuration _max_time,
      const GeoPoint &_location_predicted,
      const double _altitude_predicted,
      const FlatProjection &_projection,
      const double _max_alt = -1)
    :state(_state),
     perf(_perf),
     warning_manager(_warning_manager),
     warning_state(_warning_state),
     max_time(_max_time),
     location_predicted(_location_predicted),
     projection(_projection),
     max_alt(_max_alt),
     path{_state.location, _location_predicted,
          _state.location.Distance(_location_predicted),
          _state.altitude, _altitude_predicted}
  {
  }

  /**
   * Check whether this intersection should be added to, or updated in, the warning manager
   *
   * @param airspace Airspace corresponding to current intersection
   */
  void Intersection(ConstAirspacePtr &airspace_ptr) noexcept {
    try {
      const auto &airspace = *airspace_ptr;
      if (!airspace.IsActive())
        return;

      if (!(warning_manager.GetConfig()
              .IsClassEnabled(airspace.GetClassOrType()) ||
            warning_manager.GetConfig()
              .IsClassEnabled(airspace.GetTypeOrClass())) ||
          ExcludeAltitude(airspace))
        return;

      AirspaceWarning *warning =
        warning_manager.GetWarningPtr(airspace);

      /* Compute distance interval along predicted path.
         Always captured regardless of state acceptance,
         because clearance suppression needs intervals from
         all prediction methods. */
      AirspaceWarningInterval iv =
        AirspaceWarningInterval::Invalid();

      if (!mode_inside) {
        if (!intersections.empty()) {
          const auto &p = intersections.front();
          double d0 = state.location.Distance(p.first);
          double d1 = state.location.Distance(p.second);

          assert(d0 <= d1);
          if (d0 < d1)
            iv = {{d0, p.first}, {d1, p.second}};
          else {
            /* d0 == d1: two cases share this branch.
               (A) Polygon "enters, endpoint inside": DistinctIntersection
               Use location_predicted as the interval end
               (B) Circle tangent/near-tangent: IntersectOriginCircle
               yields f_p1==f_p2 when det approx. 0, only one point reaches the
               sorter, all() emits (T,T).  The path does not genuinely
               enter, so no interval should be generated.
               Distinguish by checking whether location_predicted is
               inside the airspace (true for A, false for B). */
            if (airspace.Inside(location_predicted)) {
              const double d_end =
                state.location.Distance(location_predicted);
              iv = {{d0, p.first}, {d_end, location_predicted}};
            }
          }
        }
      } else {
        /* Aircraft is inside this airspace. Find exit
           along the predicted path direction. */
        auto isv = airspace.Intersects(
          state.location, location_predicted, projection);

        /* all() returns inside-segments as (entry, exit) pairs.
           When state.location is on the boundary, all() may emit a
           degenerate (state.location, state.location) pair: for
           circles Inside() uses <=, and for either shape the coarse
           (~111 m) integer projection can snap the position onto an
           edge.  Skip those and use the first genuine exit. */
        const GeoPoint *exit_pt = nullptr;
        for (const auto &seg : isv) {
          if (seg.second != state.location) {
            exit_pt = &seg.second;
            break;
          }
        }

        if (exit_pt != nullptr) {
          double d = state.location.Distance(*exit_pt);
          iv = {{0, state.location}, {d, *exit_pt}};
        } else if (airspace.Inside(location_predicted)) {
          /* No boundary crossing and the predicted endpoint is also
             inside: the entire predicted path lies within the
             airspace. */
          double len =
            state.location.Distance(location_predicted);
          iv = {{0, state.location},
                {len, location_predicted}};
        } else {
          /* No genuine exit found, yet the predicted endpoint is
             outside the airspace. This happens when state.location
             lies on the airspace boundary after the coarse integer
             projection: the real exit is at t==0 and gets dropped.
             Represent the inside segment by the projection's grid 
             resolution rather than spuriously (avoiding a one-cycle
             warning on exit). */
          const double d = projection.GetApproximateScale();
          const GeoPoint exit =
            state.location.IntermediatePoint(location_predicted, d);
          iv = {{0, state.location}, {d, exit}};
        }
      }

      /* Clip the 2D-projected interval to the sub-segment where
         the aircraft's predicted altitude profile is in this
         airspace's vertical band. */
      if (iv.IsValid())
        iv = ClipByAltitudeBand(iv, path,
                                airspace.GetBaseAltitude(state),
                                airspace.GetTopAltitude(state));

      if (!iv.IsValid() && warning == nullptr)
        return;

      /* Cleared airspaces keep their interval even when this
         prediction is not a warning.  Other airspaces wait until
         the intercept is accepted, so a rejected solution does not
         leave a WARNING_CLEAR entry for WarningLive() to delete. */
      if (warning == nullptr && iv.IsValid() &&
          warning_manager.GetCleared(airspace))
        warning = warning_manager.GetNewWarningPtr(airspace_ptr);

      if (warning != nullptr && iv.IsValid())
        warning->SetInterval(warning_state, iv);

      if (warning != nullptr &&
          !warning->IsStateAccepted(warning_state))
        return;

      AirspaceInterceptSolution solution;
      if (mode_inside)
        solution = airspace.Intercept(
          state, perf, state.location, state.location);
      else
        solution = Intercept(airspace, state, perf);

      if (!solution.IsValid() || solution.elapsed_time > max_time)
        return;

      if (warning == nullptr) {
        warning =
          warning_manager.GetNewWarningPtr(std::move(airspace_ptr));
        if (iv.IsValid())
          warning->SetInterval(warning_state, iv);
      }

      warning->UpdateSolution(warning_state, solution);
      found = true;
    } catch (const std::exception &e) {
      LogFormat("Airspace intersection failed: %s", e.what());
#ifndef NDEBUG
      PrintException(e);
#else
      (void)e;
#endif
    } catch (...) {
      LogError(std::current_exception(), "Airspace intersection failed");
#ifndef NDEBUG
      PrintException(std::current_exception());
#endif
    }
  }

  void Visit(ConstAirspacePtr as) noexcept override {
    Intersection(as);
  }

  bool Found() const {
    return found;
  }

  void SetMode(bool m) {
    mode_inside = m;
    /* Drop any stale intersections from the prior pass: Otherwise 
    outdated intersction info will not be cleared in the inside pass stage */
    SetIntersections({});
  }

private:
  bool ExcludeAltitude(const AbstractAirspace &airspace) {
    if (max_alt <= 0)
      return false;

    return airspace.GetBaseAltitude(state) > max_alt;
  }
};


bool
AirspaceWarningManager::UpdatePredicted(const AircraftState& state,
                                        const GeoPoint &location_predicted,
                                        const double altitude_predicted,
                                        const AirspaceAircraftPerformance &perf,
                                        const AirspaceWarning::State warning_state,
                                        const FloatDuration max_time) noexcept
{
  // this is the time limit of intrusions, beyond which we are not interested.
  // it can be the minimum of the user set warning time, or the time of the 
  // task segment

  const auto max_time_limit = std::min(FloatDuration{config.warning_time},
                                       max_time);

  // the ceiling is the max height for predicted intrusions, given
  // that you may be climbing.  the ceiling is nominally set at 1000m
  // above the current altitude, but the 1000m margin should be at
  // least as big as config.AltWarningMargin since if the airspace is
  // visible according to that display mode, it should have warnings
  // collected for it.  It is very unlikely users will have more than 1000m
  // in AltWarningMargin anyway.

  const auto ceiling = state.altitude
    + std::max((unsigned)1000, config.altitude_warning_margin);

  AirspaceIntersectionWarningVisitor visitor(
    state, perf, *this, warning_state, max_time_limit,
    location_predicted, altitude_predicted,
    GetProjection(), ceiling);

  airspaces.VisitIntersecting(state.location, location_predicted, visitor);

  visitor.SetMode(true);

  for (const auto &i : airspaces.QueryInside(state.location)) {
    visitor.Visit(i.GetAirspacePtr());
  }

  return visitor.Found();
}


bool 
AirspaceWarningManager::UpdateTask(const AircraftState &state,
                                   const GlidePolar &glide_polar,
                                   const TaskStats &task_stats)
{
  if (!glide_polar.IsValid())
    return false;

  const ElementStat &current_leg = task_stats.current_leg;

  if (!task_stats.task_valid || !current_leg.location_remaining.IsValid())
    return false;

  const GlideResult &solution = current_leg.solution_remaining;
  if (!solution.IsOk() || !solution.IsAchievable())
    /* glide solver failed, cannot continue */
    return false;

  const AirspaceAircraftPerformance perf_task(glide_polar,
                                              current_leg.solution_remaining);
  GeoPoint location_tp = current_leg.location_remaining;
  const auto time_remaining = solution.time_elapsed;

  const GeoVector vector(state.location, location_tp);
  auto max_distance = config.warning_time.count() * glide_polar.GetVMax();
  if (vector.distance > max_distance)
    /* limit the distance to what our glider can actually fly within
       the configured warning time */
    location_tp = state.location.IntermediatePoint(location_tp, max_distance);

  /* TASK: pass state.altitude as a flat-profile fallback.
     Deriving the predicted altitude at location_tp from the leg
     glide solution requires careful interpretation of GlideResult
     fields and the max-distance truncation; left for a follow-up.
     Flat profile here matches today's behaviour for TASK warnings. */
  return UpdatePredicted(state, location_tp, state.altitude,
                          perf_task,
                          AirspaceWarning::WARNING_TASK, time_remaining);
}


bool
AirspaceWarningManager::UpdateFilter(const AircraftState& state, const bool circling)
{
  // update both filters even though we are using only one
  cruise_filter.Update(state);
  circling_filter.Update(state);

  const AircraftState predicted = circling
    ? circling_filter.GetPredictedState(prediction_time_filter)
    : cruise_filter.GetPredictedState(prediction_time_filter);

  if (circling)
    return UpdatePredicted(state, predicted.location, predicted.altitude,
                           AirspaceAircraftPerformance(circling_filter),
                           AirspaceWarning::WARNING_FILTER,
                           prediction_time_filter);
  else
    return UpdatePredicted(state, predicted.location, predicted.altitude,
                           AirspaceAircraftPerformance(cruise_filter),
                           AirspaceWarning::WARNING_FILTER,
                           prediction_time_filter);
}


bool 
AirspaceWarningManager::UpdateGlide(const AircraftState &state,
                                    const GlidePolar &glide_polar)
{
  if (!glide_polar.IsValid())
    return false;

  const AircraftState predicted =
    state.GetPredictedState(prediction_time_glide);

  const AirspaceAircraftPerformance perf_glide(glide_polar);
  return UpdatePredicted(state, predicted.location, predicted.altitude,
                          perf_glide,
                          AirspaceWarning::WARNING_GLIDE, prediction_time_glide);
}

bool
AirspaceWarningManager::UpdateInside(const AircraftState& state,
                                     const GlidePolar &glide_polar)
{
  if (!glide_polar.IsValid())
    return false;

  bool found = false;

  for (const auto &i : airspaces.QueryInside(state.location)) {
    const auto airspace = i.GetAirspacePtr();

    const AltitudeState &altitude = state;
    if (// ignore inactive airspaces
        !airspace->IsActive() ||
        !(config.IsClassEnabled(airspace->GetClassOrType()) || config.IsClassEnabled(airspace->GetTypeOrClass())) ||
        !airspace->Inside(altitude))
      continue;

    AirspaceWarning *warning = GetWarningPtr(*airspace);

    if (warning == nullptr ||
        warning->IsStateAccepted(AirspaceWarning::WARNING_INSIDE)) {
      GeoPoint c = airspace->ClosestPoint(state.location, GetProjection());
      const AirspaceAircraftPerformance perf_glide(glide_polar);
      const AirspaceInterceptSolution solution =
        airspace->Intercept(state, c, GetProjection(), perf_glide);

      if (warning == nullptr)
        warning = GetNewWarningPtr(airspace);

      warning->UpdateSolution(AirspaceWarning::WARNING_INSIDE, solution);
      found = true;
    }
  }

  return found;
}

void
AirspaceWarningManager::Resort() noexcept
{
  warnings.sort();
}

bool
AirspaceWarningManager::CoverOutsideAirspaces() noexcept
{
  if (!last_location.IsValid())
    return false;

  bool covered = false;
  for (const auto &cleared : warnings) {
    if (!cleared.IsCleared())
      continue;
    if (!cleared.GetAirspace().Inside(last_location))
      continue;

    /* The top is a boundary even though the horizontal distance is
       zero. Warn once the climb reaches it within the warning time. */
    AltitudeState alt_state{};
    alt_state.altitude = last_altitude;
    alt_state.altitude_agl = last_altitude_agl;
    const double top = cleared.GetAirspace().GetTopAltitude(alt_state);
    if (last_altitude >= top)
      continue;
    if (last_vario > 0 &&
        (top - last_altitude) / last_vario <=
        double(config.warning_time.count()))
      continue;

    for (auto &other : warnings) {
      if (&other == &cleared || other.IsCleared() ||
          other.IsCoveredByClearance() || !other.IsWarning())
        continue;
      /* A boundary ahead is the exit from the clearance. Silence
         only an airspace that already contains the aircraft and has
         no horizontal distance. The cleared area's centre is the
         wrong test: a large polygon can stick out of the airspace
         above the aircraft. */
      const auto &sol = other.GetSolution();
      if (!sol.IsValid() || sol.distance > 0)
        continue;
      if (!other.GetAirspace().Inside(last_location))
        continue;
      other.SetCoveredByClearance(true);
      covered = true;
    }
  }
  return covered;
}

bool
AirspaceWarningManager::AckOutsideAirspaces(const AbstractAirspace &acked) noexcept
{
  if (!last_location.IsValid())
    return false;

  if (!acked.Inside(last_location))
    return false;

  AltitudeState alt_state{};
  alt_state.altitude = last_altitude;
  alt_state.altitude_agl = last_altitude_agl;
  const double band_base = acked.GetBaseAltitude(alt_state);
  const double band_top = acked.GetTopAltitude(alt_state);

  bool changed = false;
  for (auto &other : warnings) {
    if (&other.GetAirspace() == &acked || other.GetAckDay() ||
        !other.IsWarning())
      continue;
    const auto &sol = other.GetSolution();
    if (!sol.IsValid() || sol.distance > 0)
      continue;
    if (!other.GetAirspace().Inside(last_location))
      continue;

    /* Share a height band. A floor at or above this ceiling
       is still ahead. */
    const double other_base =
      other.GetAirspace().GetBaseAltitude(alt_state);
    const double other_top =
      other.GetAirspace().GetTopAltitude(alt_state);
    if (other_base >= band_top || other_top <= band_base)
      continue;

    other.AcknowledgeDay(true);
    ack_day_airspaces.insert(&other.GetAirspace());
    changed = true;
  }
  return changed;
}

void
AirspaceWarningManager::Acknowledge(ConstAirspacePtr airspace) noexcept
{
  auto *w = GetWarningPtr(*airspace);
  if (w == nullptr || !w->IsActive())
    return;

  w->Acknowledge();
  Resort();
  ++serial;
}

void
AirspaceWarningManager::AcknowledgeWarning(ConstAirspacePtr airspace,
                                           const bool set)
{
  AirspaceWarning *warning = nullptr;
  if (set) {
    warning = &GetWarning(std::move(airspace));
  } else {
    // Avoid creating a warning when just clearing an acknowledgement.
    warning = GetWarningPtr(*airspace);
    if (warning == nullptr)
      return;
  }

  const bool was_acknowledged = warning->IsWarningAcknowledged();
  warning->AcknowledgeWarning(set);
  if (warning->IsWarningAcknowledged() == was_acknowledged)
    return;

  Resort();
  ++serial;
}

void
AirspaceWarningManager::AcknowledgeInside(ConstAirspacePtr airspace,
                                          const bool set)
{
  AirspaceWarning *warning = nullptr;
  if (set) {
    warning = &GetWarning(std::move(airspace));
  } else {
    // Avoid creating a warning when just clearing an acknowledgement.
    warning = GetWarningPtr(*airspace);
    if (warning == nullptr)
      return;
  }

  const bool was_acknowledged = warning->IsInsideAcknowledged();
  warning->AcknowledgeInside(set);
  if (warning->IsInsideAcknowledged() == was_acknowledged)
    return;

  Resort();
  ++serial;
}

void
AirspaceWarningManager::AcknowledgeDay(ConstAirspacePtr airspace,
                                       const bool set)
{
  const AbstractAirspace *const raw = airspace.get();
  const char *const key = NotamDayAckKey(*airspace);
  const std::string_view key_view =
    key != nullptr ? std::string_view{key} : std::string_view{};
  const bool was_member = key != nullptr &&
    notam_day_ack_by_station.contains(key_view);

  AirspaceWarning *warning = nullptr;
  if (set) {
    warning = &GetWarning(std::move(airspace));
  } else {
    // Avoid creating a warning when just clearing an acknowledgement.
    warning = GetWarningPtr(*airspace);
    if (warning == nullptr && key != nullptr)
      warning = FindWarningByNotamDayAckKey(key_view);
    if (warning == nullptr && key != nullptr && !was_member)
      return;
  }

  const bool was_acknowledged = warning != nullptr && warning->GetAckDay();

  bool membership_changed = false;
  if (key != nullptr) {
    if (set && !was_member)
      membership_changed = notam_day_ack_by_station.emplace(key).second;
    else if (!set && was_member)
      membership_changed = notam_day_ack_by_station.erase(key) > 0;
  }

  bool ack_changed = false;
  if (key != nullptr) {
    for (auto &candidate : warnings) {
      const char *const candidate_key =
        NotamDayAckKey(candidate.GetAirspace());
      if (candidate_key == nullptr || key_view != candidate_key)
        continue;

      const bool old_acknowledged = candidate.GetAckDay();
      candidate.AcknowledgeDay(set);
      ack_changed |= candidate.GetAckDay() != old_acknowledged;
    }
  } else if (warning != nullptr) {
    warning->AcknowledgeDay(set);
    ack_changed = warning->GetAckDay() != was_acknowledged;
  }

  if (raw != nullptr) {
    if (set)
      ack_day_airspaces.insert(raw);
    else
      ack_day_airspaces.erase(raw);
  }
  if (key != nullptr) {
    for (const auto &candidate : warnings) {
      const char *const candidate_key =
        NotamDayAckKey(candidate.GetAirspace());
      if (candidate_key == nullptr || key_view != candidate_key)
        continue;
      if (set)
        ack_day_airspaces.insert(&candidate.GetAirspace());
      else
        ack_day_airspaces.erase(&candidate.GetAirspace());
    }
  }

  const bool outside = set && raw != nullptr &&
    AckOutsideAirspaces(*raw);

  if (!membership_changed && !ack_changed && !outside)
    return;

  Resort();
  ++serial;
}

bool
AirspaceWarningManager::GetAckDay(const AbstractAirspace &airspace) const noexcept
{
  if (const char *key = NotamDayAckKey(airspace);
      key != nullptr &&
      notam_day_ack_by_station.contains(key))
    return true;

  if (ack_day_airspaces.contains(&airspace))
    return true;

  const AirspaceWarning *warning = GetWarningPtr(airspace);
  return warning != nullptr && warning->GetAckDay();
}

void
AirspaceWarningManager::SetCleared(ConstAirspacePtr airspace,
                                   const bool set)
{
  const char *const key = NotamDayAckKey(*airspace);
  bool membership_changed = false;
  if (key != nullptr) {
    if (set)
      membership_changed = notam_day_cleared_by_station.emplace(key).second;
    else
      membership_changed = notam_day_cleared_by_station.erase(key) > 0;
  }

  auto &warning = GetWarning(std::move(airspace));
  bool flag_changed = false;
  if (key != nullptr) {
    /* Every polygon of this NOTAM already in the list must carry
       the flag.  IsCleared() does not consult the station set. */
    const std::string_view key_view{key};
    for (auto &candidate : warnings) {
      const char *const candidate_key =
        NotamDayAckKey(candidate.GetAirspace());
      if (candidate_key == nullptr || key_view != candidate_key)
        continue;
      if (candidate.IsCleared() == set)
        continue;
      candidate.SetCleared(set);
      flag_changed = true;
    }
  } else if (warning.IsCleared() != set) {
    warning.SetCleared(set);
    flag_changed = true;
  }

  const bool covered = set && CoverOutsideAirspaces();

  // The renderer fill cache keys on the manager serial; without this
  // bump, toggling clearance on an existing warning would not refresh
  // the cached fill on the non-GL renderer.
  if (!membership_changed && !flag_changed && !covered)
    return;

  Resort();
  ++serial;
}

bool
AirspaceWarningManager::GetCleared(const AbstractAirspace &airspace) const noexcept
{
  if (const char *key = NotamDayAckKey(airspace);
      key != nullptr && notam_day_cleared_by_station.contains(key))
    return true;

  const AirspaceWarning *warning = GetWarningPtr(airspace);
  return warning != nullptr && warning->IsCleared();
}

bool
AirspaceWarningManager::IsActive(const AbstractAirspace &airspace) const noexcept
{
  return airspace.IsActive() && (config.IsClassEnabled(airspace.GetClassOrType()) || config.IsClassEnabled(airspace.GetTypeOrClass())) &&
    !GetAckDay(airspace);
}

void
AirspaceWarningManager::AcknowledgeAll()
{
  bool changed = false;
  for (auto &w : warnings) {
    if (!w.IsActive())
      continue;

    w.AcknowledgeWarning(true);
    w.AcknowledgeInside(true);
    changed = true;
  }

  if (!changed)
    return;

  Resort();
  ++serial;
}

namespace {

constexpr AirspaceWarning::State kPredictionMethods[] = {
  AirspaceWarning::WARNING_GLIDE,
  AirspaceWarning::WARNING_FILTER,
  AirspaceWarning::WARNING_TASK,
};

/** Capacity for small stack-allocated cleared-airspace buffers. */
constexpr std::size_t kClearedBufCap = 32;

[[gnu::pure]]
static AirspaceAircraftPerformance
PerfFor(AirspaceWarning::State method,
        const GlidePolar &glide_polar,
        const AircraftStateFilter &cruise_filter,
        const AircraftStateFilter &circling_filter,
        bool circling,
        const TaskStats &task_stats) noexcept
{
  switch (method) {
  case AirspaceWarning::WARNING_GLIDE:
    if (glide_polar.IsValid())
      return AirspaceAircraftPerformance{glide_polar};
    break;

  case AirspaceWarning::WARNING_FILTER:
    return circling
      ? AirspaceAircraftPerformance{circling_filter}
      : AirspaceAircraftPerformance{cruise_filter};

  case AirspaceWarning::WARNING_TASK:
    if (glide_polar.IsValid() && task_stats.task_valid) {
      const auto &solution =
        task_stats.current_leg.solution_remaining;
      if (solution.IsOk() && solution.IsAchievable())
        return AirspaceAircraftPerformance{glide_polar, solution};
    }
    break;

  case AirspaceWarning::WARNING_CLEAR:
  case AirspaceWarning::WARNING_INSIDE:
    break;
  }
  return AirspaceAircraftPerformance{
    AirspaceAircraftPerformance::Simple{}};
}

/**
 * Sort a small array of warning pointers by the entry distance
 * of their interval for the given prediction method.
 */
static void
SortByEntryDistance(AirspaceWarning **first,
                    AirspaceWarning **last,
                    AirspaceWarning::State method) noexcept
{
  std::sort(first, last,
            [method](const AirspaceWarning *a,
                     const AirspaceWarning *b) {
              return a->GetInterval(method).entry.distance <
                     b->GetInterval(method).entry.distance;
            });
}

} // namespace

bool
AirspaceWarningManager::IsThinClearanceCorridor(
    const AirspaceWarningInterval &iv,
    const AbstractAirspace &offending,
    const AircraftState &state,
    const double tolerance) const noexcept
{
  /* Interval arithmetic works on the coarse (~111 m) integer
     projection.  When a non-cleared airspace shares a
     near-coincident boundary with a cleared one, the residual that
     survives subtraction can be a thin strip hugging the cleared
     boundary: the two boundaries are separate in float geometry but
     collapse (or fail to nest) on the integer grid.  Such strips are
     digitisation/projection artifacts -- no real airspace leaves a
     sub-grid-cell corridor between a sector and its enclosing
     airspace -- so a warning whose entire relevant interval runs
     inside or within one tolerance of a cleared airspace is
     suppressed.  A genuine intrusion cannot be suppressed this way:
     some sample of its interval lies farther than the tolerance from
     every cleared airspace. */

  const FlatProjection &projection = GetProjection();

  /* Restrict the mechanism to the near-coincident-boundary case:
     the interval must start within one tolerance of the offending
     airspace's own boundary.  Checked only for the first point
     because offending airspaces (CTRs etc.) may have many vertices,
     while cleared airspaces are typically simple. */
  if (offending.DistanceToBoundary(iv.entry.location, projection) >
      tolerance)
    return false;

  /* Collect cleared airspaces whose vertical band contains the
     current altitude.  Clearance is whole-airspace; the short
     prediction window means the current altitude is a good proxy
     along the path. */
  std::array<const AbstractAirspace *, kClearedBufCap> cleared{};
  std::size_t n_cleared = 0;
  for (const auto &c : warnings) {
    if (!c.IsCleared()) continue;
    const auto &as = c.GetAirspace();
    if (state.altitude < as.GetBaseAltitude(state) ||
        state.altitude > as.GetTopAltitude(state))
      continue;
    if (n_cleared < cleared.size())
      cleared[n_cleared++] = &as;
  }
  if (n_cleared == 0)
    return false;

  const auto near_cleared = [&](const GeoPoint &p) {
    for (std::size_t i = 0; i < n_cleared; ++i) {
      const auto &as = *cleared[i];
      /* Inside() first: exact float geometry, and the common case
         for samples genuinely inside the cleared airspace. */
      if (as.Inside(p) ||
          as.DistanceToBoundary(p, projection) <= tolerance)
        return true;
    }
    return false;
  };

  const auto point_at = [&iv](double d) {
    return iv.entry.location.IntermediatePoint(iv.exit.location,
                                               d - iv.entry.distance);
  };

  /* Walk no farther than the warning horizon; intervals beyond it
     cannot alert within warning_time anyway. */
  const FloatDuration warning_time{config.warning_time};
  const double d_last =
    std::max(iv.entry.distance,
             std::min(iv.exit.distance,
                      std::max(tolerance,
                               state.ground_speed *
                                 warning_time.count())));

  /* Interval endpoints come from the coarse integer projection and
     can overshoot the real (float) boundary of the offending
     airspace by up to a grid cell -- most notably the synthetic
     one-cell interval created when the aircraft sits on the exit
     edge.  A sample that is not genuinely inside the offending
     airspace represents no intrusion, so it is disregarded instead
     of being required to be near a clearance. */
  const auto sample_ok = [&](const GeoPoint &p) {
    return near_cleared(p) || !offending.Inside(p);
  };

  /* Sample order chosen for early-out: first point, then the far
     end (fails fast for genuine deep intrusions), then steps of one
     tolerance from near to far.  The first point must be near a
     clearance unconditionally: it anchors the corridor to an actual
     clearance, so an unrelated interval (e.g. a tangential graze of
     the offending airspace far away from any cleared airspace)
     cannot be suppressed. */
  if (!near_cleared(iv.entry.location))
    return false;
  if (!sample_ok(point_at(d_last)))
    return false;
  for (double d = iv.entry.distance + tolerance; d < d_last;
       d += tolerance)
    if (!sample_ok(point_at(d)))
      return false;

  return true;
}

void
AirspaceWarningManager::ProcessClearanceIntervals(
    const AircraftState &state,
    const GlidePolar &glide_polar,
    const bool circling,
    const TaskStats &task_stats) noexcept
{
  // Fast path + collect cleared airspaces the aircraft is
  // physically inside.
  std::array<AirspaceWarning *, kClearedBufCap> cleared_inside_buf{};
  std::size_t n_cleared_inside = 0;
  bool any_cleared = false;

  for (auto &w : warnings) {
    if (!w.IsCleared()) continue;
    any_cleared = true;
    if (w.GetAirspace().Inside(state) &&
        n_cleared_inside < cleared_inside_buf.size())
      cleared_inside_buf[n_cleared_inside++] = &w;
  }
  if (!any_cleared) return;

  const FloatDuration warning_time{config.warning_time};

  /* Tolerance for clearance interval arithmetic.  Use the integer
     flat projection's grid resolution (~111 m) instead of the
     smaller kMinFragmentLength: interval endpoints are computed on
     that grid, so fragments below one grid cell are below the
     geometric resolution of the data they are derived from. */
  const double tolerance = GetProjection().GetApproximateScale();

  /* Warnings that step 1 downgraded out of WARNING_INSIDE. Step 2
     re-processes these (they're no longer INSIDE), but should not
     re-subtract clearances already applied in step 1 (i.e. those
     the aircraft is currently 3D-inside) since that's redundant.
     Bounded by the number of non-cleared INSIDE warnings, normally
     0-3 in practice; if it exceeds capacity we fall back to the
     regular re-subtraction. */
  std::array<AirspaceWarning *, 16> step1_downgraded{};
  std::size_t n_step1_downgraded = 0;

  /* Shared by the inside pass and the approach pass.  Each pass
     chooses which clearances apply; these only subtract intervals
     and rebuild the intercept from what remains. */
  struct Residual {
    AirspaceWarning::State method;
    double distance;
    GeoPoint location;
  };

  const auto subtract_cleared =
    [](AirspaceWarningInterval iv, AirspaceWarning **buf, std::size_t n,
       AirspaceWarning::State method, double tolerance) noexcept {
      SortByEntryDistance(buf, buf + n, method);
      for (std::size_t i = 0; i < n; ++i) {
        SubtractInterval(iv, buf[i]->GetInterval(method), tolerance);
        if (!iv.IsValid())
          break;
      }
      return iv;
    };

  const auto interval_changed =
    [](const AirspaceWarningInterval &before,
       const AirspaceWarningInterval &after) noexcept {
      return !after.IsValid()
        || after.entry.distance != before.entry.distance
        || after.exit.distance != before.exit.distance;
    };

  const auto apply_nearest =
    [&](AirspaceWarning &w, Residual *residuals, std::size_t n_res,
        bool force_state) noexcept {
      std::sort(residuals, residuals + n_res,
                [](const Residual &a, const Residual &b) {
                  return a.distance < b.distance;
                });

      for (std::size_t i = 0; i < n_res; ++i) {
        const auto &r = residuals[i];
        const AirspaceAircraftPerformance perf = PerfFor(
          r.method, glide_polar, cruise_filter,
          circling_filter, circling, task_stats);
        AirspaceInterceptSolution sol =
          w.GetAirspace().Intercept(state, perf,
                                    r.location, r.location);
        if (sol.IsValid() && sol.elapsed_time <= warning_time) {
          if (force_state)
            w.ForceState(r.method);
          w.SetSolution(sol);
          return true;
        }
      }
      return false;
    };

  // Step 1: convert WARNING_INSIDE warnings of non-cleared
  // airspaces.  Subtraction applies the coverage of cleared
  // airspaces the aircraft is physically inside; the corridor
  // check additionally drops thin artifact residuals along
  // near-coincident boundaries (which can exist before the
  // aircraft has entered the cleared airspace itself).
  for (auto &w : warnings) {
    if (w.IsCleared()) continue;
    if (w.GetWarningState() != AirspaceWarning::WARNING_INSIDE)
      continue;

    bool any_consumed_by_clearance = false;
    for (const auto m : kPredictionMethods) {
      AirspaceWarningInterval iv = w.GetInterval(m);
      if (!iv.IsValid()) continue;

      const AirspaceWarningInterval iv_before = iv;

      std::array<AirspaceWarning *, kClearedBufCap> buf{};
      std::size_t n = 0;
      for (std::size_t i = 0; i < n_cleared_inside; ++i) {
        AirspaceWarning *c = cleared_inside_buf[i];
        if (c->HasInterval(m))
          buf[n++] = c;
      }
      iv = subtract_cleared(iv, buf.data(), n, m, tolerance);

      /* Near-coincident boundaries snap together on the integer
         projection and leave a short interval that a clearance
         still consumed. */
      if (interval_changed(iv_before, iv))
        any_consumed_by_clearance = true;

      w.SetInterval(m, iv);
    }

    // Collect surviving intervals across methods, sort by
    // nearest entry distance, and try each in turn until one
    // produces a valid intercept solution.
    std::array<Residual, std::size(kPredictionMethods)> residuals;
    std::size_t n_res = 0;
    for (const auto m : kPredictionMethods) {
      const AirspaceWarningInterval &iv = w.GetInterval(m);
      if (!iv.IsValid()) continue;
      /* A sub-tolerance sliver along a cleared boundary counts as
         consumed, otherwise the inside warning is left untouched. */
      if (IsThinClearanceCorridor(iv, w.GetAirspace(), state,
                                  tolerance)) {
        any_consumed_by_clearance = true;
        w.SetInterval(m, AirspaceWarningInterval::Invalid());
        continue;
      }
      if (iv.Length() < tolerance) continue;
      residuals[n_res++] = {m, iv.entry.distance,
                            iv.entry.location};
    }

    /* No clearance touched any interval (neither subtraction nor
       corridor check): leave the genuine INSIDE warning alone.
       Without this, an untouched inside interval (entry.distance
       == 0) would be re-resolved below into a zero-distance
       approach warning. */
    if (!any_consumed_by_clearance)
      continue;

    if (n_res == 0) {
      w.SetCoveredByClearance(true);
      continue;
    }

    if (apply_nearest(w, residuals.data(), n_res, true)) {
      if (n_step1_downgraded < step1_downgraded.size())
        step1_downgraded[n_step1_downgraded++] = &w;
    } else {
      // Every interval is too far / unreachable; treat as
      // covered.
      w.SetCoveredByClearance(true);
    }
  }

  // Step 2: clip approach warnings (state in GLIDE/FILTER/TASK)
  // by cleared coverage along the same method's predicted path.
  for (auto &w : warnings) {
    if (w.IsCleared()) continue;
    const auto cur_state = w.GetWarningState();
    if (cur_state == AirspaceWarning::WARNING_CLEAR ||
        cur_state == AirspaceWarning::WARNING_INSIDE)
      continue;

    /* Did step 1 already downgrade this warning?  If so, the
       cleared were already subtracted in step 1 */
    const bool step1_handled = std::find(
      step1_downgraded.begin(),
      step1_downgraded.begin() + n_step1_downgraded,
      &w) != step1_downgraded.begin() + n_step1_downgraded;

    bool any_changed = false;
    std::array<Residual, std::size(kPredictionMethods)> residuals;
    std::size_t n_res = 0;

    for (const auto m : kPredictionMethods) {
      AirspaceWarningInterval iv = w.GetInterval(m);
      if (!iv.IsValid()) continue;
      const AirspaceWarningInterval iv_orig = iv;

      // Collect cleared with valid interval and vertical
      // overlap with W; sort near-to-far.
      std::array<AirspaceWarning *, kClearedBufCap> buf{};
      std::size_t n = 0;
      for (auto &c : warnings) {
        if (!c.IsCleared()) continue;
        if (!c.HasInterval(m)) continue;
        if (step1_handled) {
          /* Aircraft is currently inside this clearance and step
             1 already subtracted it from w's interval; skip the
             redundant subtraction. */
          bool in_inside_buf = false;
          for (std::size_t i = 0; i < n_cleared_inside; ++i)
            if (cleared_inside_buf[i] == &c) {
              in_inside_buf = true;
              break;
            }
          if (in_inside_buf) continue;
        }
        if (n >= buf.size()) break;
        buf[n++] = &c;
      }
      iv = subtract_cleared(iv, buf.data(), n, m, tolerance);

      const bool changed = interval_changed(iv_orig, iv);
      if (changed) any_changed = true;

      if (!iv.IsValid() ||
          (changed && iv.Length() < tolerance)) {
        w.SetInterval(m, AirspaceWarningInterval::Invalid());
      } else if (IsThinClearanceCorridor(iv, w.GetAirspace(), state,
                                         tolerance)) {
        /* Interval subtraction left this (possibly unchanged because
           the integer intervals failed to overlap or never did), but
           the whole warning-relevant part runs along a cleared
           boundary: thin artifact corridor from integer-projection
           non-nesting of near-coincident boundaries. */
        any_changed = true;
        w.SetInterval(m, AirspaceWarningInterval::Invalid());
      } else {
        w.SetInterval(m, iv);
        residuals[n_res++] = {m, iv.entry.distance,
                              iv.entry.location};
      }
    }

    if (!any_changed) continue;

    if (n_res == 0) {
      // All approach intervals fully covered by clearance.
      w.SetCoveredByClearance(true);
      continue;
    }

    // Rebuild the solution at the nearest surviving interval whose
    // Intercept solves; try the next-nearest method if it fails.
    if (!apply_nearest(w, residuals.data(), n_res, false))
      w.SetCoveredByClearance(true);
  }

  CoverOutsideAirspaces();
}
