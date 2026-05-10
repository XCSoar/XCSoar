// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningManager.hpp"
#include "Geo/GeoVector.hpp"
#include "Airspaces.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceIntersectionVisitor.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "Task/Stats/TaskStats.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>

static constexpr double CRUISE_FILTER_FACT = 0.5;

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
  warnings.clear();
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
AirspaceWarningManager::GetWarning(ConstAirspacePtr airspace) noexcept
{
  AirspaceWarning* warning = GetWarningPtr(*airspace);
  if (warning)
    return *warning;

  // not found, create new entry
  ++serial;
  warnings.emplace_back(std::move(airspace));
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
AirspaceWarningManager::GetNewWarningPtr(ConstAirspacePtr airspace) noexcept
{
  ++serial;
  warnings.emplace_back(airspace);
  return &warnings.back();
}

bool 
AirspaceWarningManager::Update(const AircraftState& state,
                               const GlidePolar &glide_polar,
                               const TaskStats &task_stats,
                               const bool circling,
                               const std::chrono::duration<unsigned> dt) noexcept
{
  bool changed = false;

  // update warning states
  if (airspaces.IsEmpty()) {
    // no airspaces, no warnings possible
    assert(warnings.empty());
    return false;
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
      if (it->ChangedState())
        changed = true;

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
          /* p.first == p.second: the path enters the airspace but
             the predicted endpoint lies inside it. Use the prediction
             endpoint as the interval end instead */
          const double d_end =
            state.location.Distance(location_predicted);
          iv = {{d0, p.first}, {d_end, location_predicted}};
        }
      }
    } else {
      /* Aircraft is inside this airspace. Find exit
         along the predicted path direction. */
      auto isv = airspace.Intersects(
        state.location, location_predicted, projection);
      if (isv.empty()) {
        /* entire predicted path inside airspace */
        double len =
          state.location.Distance(location_predicted);
        iv = {{0, state.location},
              {len, location_predicted}};
      } else {
        /* all() returns (entry, exit) pairs; when starting
           inside, the first pair is (start_pos, exit_point),
           so .second is the exit boundary */
        double d =
          state.location.Distance(isv.front().second);
        iv = {{0, state.location},
              {d, isv.front().second}};
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

    if (warning == nullptr)
      warning =
        warning_manager.GetNewWarningPtr(
          std::move(airspace_ptr));

    if (iv.IsValid())
      warning->SetInterval(warning_state, iv);

    /* Upgrade state and solution only when the new state
       is at least as severe as the current one */
    if (warning->IsStateAccepted(warning_state)) {
      AirspaceInterceptSolution solution;
      if (mode_inside)
        solution = airspace.Intercept(
          state, perf, state.location, state.location);
      else
        solution = Intercept(airspace, state, perf);

      if (!solution.IsValid())
        return;
      if (solution.elapsed_time > max_time)
        return;

      warning->UpdateSolution(warning_state, solution);
      found = true;
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
AirspaceWarningManager::Acknowledge(ConstAirspacePtr airspace) noexcept
{
  auto *w = GetWarningPtr(*airspace);
  if (w != nullptr)
    w->Acknowledge();
}

void
AirspaceWarningManager::AcknowledgeWarning(ConstAirspacePtr airspace,
                                           const bool set) noexcept
{
  GetWarning(std::move(airspace)).AcknowledgeWarning(set);
}

void
AirspaceWarningManager::AcknowledgeInside(ConstAirspacePtr airspace,
                                          const bool set) noexcept
{
  GetWarning(std::move(airspace)).AcknowledgeInside(set);
}

void
AirspaceWarningManager::AcknowledgeDay(ConstAirspacePtr airspace,
                                       const bool set) noexcept
{
  GetWarning(std::move(airspace)).AcknowledgeDay(set);
}

bool
AirspaceWarningManager::GetAckDay(const AbstractAirspace &airspace) const noexcept
{
  const AirspaceWarning *warning = GetWarningPtr(airspace);
  return warning != nullptr && warning->GetAckDay();
}

void
AirspaceWarningManager::SetCleared(ConstAirspacePtr airspace,
                                   const bool set) noexcept
{
  auto &warning = GetWarning(std::move(airspace));
  if (warning.IsCleared() != set) {
    warning.SetCleared(set);
    // The renderer fill cache keys on the manager serial; without this
    // bump, toggling clearance on an existing warning would not refresh
    // the cached fill on the non-GL renderer.
    ++serial;
  }
}

bool
AirspaceWarningManager::GetCleared(const AbstractAirspace &airspace) const noexcept
{
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
  for (auto &w : warnings) {
    w.AcknowledgeWarning(true);
    w.AcknowledgeInside(true);
  }
}

namespace {

constexpr AirspaceWarning::State kPredictionMethods[] = {
  AirspaceWarning::WARNING_GLIDE,
  AirspaceWarning::WARNING_FILTER,
  AirspaceWarning::WARNING_TASK,
};

/** Minimum residual path fragment length to keep, in meters. */
constexpr double kMinFragmentLength = 50.0;

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

  const bool inside_cleared = n_cleared_inside > 0;
  const FloatDuration warning_time{config.warning_time};

  /* Warnings that step 1 downgraded out of WARNING_INSIDE. Step 2
     re-processes these (they're no longer INSIDE), but should not
     re-subtract clearances already applied in step 1 (i.e. those
     the aircraft is currently 3D-inside) since that's redundant.
     Bounded by the number of non-cleared INSIDE warnings, normally
     0-3 in practice; if it exceeds capacity we fall back to the
     regular re-subtraction. */
  std::array<AirspaceWarning *, 16> step1_downgraded{};
  std::size_t n_step1_downgraded = 0;

  // Step 1: convert WARNING_INSIDE warnings of non-cleared
  // airspaces while inside another, cleared, airspace.
  if (inside_cleared) {
    for (auto &w : warnings) {
      if (w.IsCleared()) continue;
      if (w.GetWarningState() != AirspaceWarning::WARNING_INSIDE)
        continue;

      for (const auto m : kPredictionMethods) {
        AirspaceWarningInterval iv = w.GetInterval(m);
        if (!iv.IsValid()) continue;

        std::array<AirspaceWarning *, kClearedBufCap> buf{};
        std::size_t n = 0;
        for (std::size_t i = 0; i < n_cleared_inside; ++i) {
          AirspaceWarning *c = cleared_inside_buf[i];
          if (c->HasInterval(m))
            buf[n++] = c;
        }
        SortByEntryDistance(buf.data(), buf.data() + n, m);
        for (std::size_t i = 0; i < n; ++i) {
          SubtractInterval(iv, buf[i]->GetInterval(m));
          if (!iv.IsValid()) break;
        }
        w.SetInterval(m, iv);
      }

      // Collect surviving intervals across methods, sort by
      // nearest entry distance, and try each in turn until one
      // produces a valid intercept solution.
      struct Residual {
        AirspaceWarning::State method;
        double distance;
        GeoPoint location;
      };
      std::array<Residual, std::size(kPredictionMethods)> residuals;
      std::size_t n_res = 0;
      for (const auto m : kPredictionMethods) {
        const AirspaceWarningInterval &iv = w.GetInterval(m);
        if (!iv.IsValid()) continue;
        if (iv.Length() < kMinFragmentLength) continue;
        residuals[n_res++] = {m, iv.entry.distance,
                              iv.entry.location};
      }

      if (n_res == 0) {
        // No meaningful remaining intervals along any method.
        // Aircraft is fully covered by cleared coverage.
        w.SetCoveredByClearance(true);
        continue;
      }

      std::sort(residuals.begin(), residuals.begin() + n_res,
                [](const Residual &a, const Residual &b) {
                  return a.distance < b.distance;
                });

      bool resolved = false;
      for (std::size_t i = 0; i < n_res; ++i) {
        const auto &r = residuals[i];
        const AirspaceAircraftPerformance perf = PerfFor(
          r.method, glide_polar, cruise_filter,
          circling_filter, circling, task_stats);
        AirspaceInterceptSolution sol =
          w.GetAirspace().Intercept(state, perf,
                                    r.location, r.location);
        if (sol.IsValid() && sol.elapsed_time <= warning_time) {
          w.ForceState(r.method);
          w.SetSolution(sol);
          resolved = true;
          if (n_step1_downgraded < step1_downgraded.size())
            step1_downgraded[n_step1_downgraded++] = &w;
          break;
        }
      }

      if (!resolved) {
        // Every interval is too far / unreachable; treat as
        // covered.
        w.SetCoveredByClearance(true);
      }
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
    struct Residual {
      AirspaceWarning::State method;
      double distance;
      GeoPoint location;
    };
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
      SortByEntryDistance(buf.data(), buf.data() + n, m);
      for (std::size_t i = 0; i < n; ++i) {
        SubtractInterval(iv, buf[i]->GetInterval(m));
        if (!iv.IsValid()) break;
      }

      const bool changed = !iv.IsValid()
        || iv.entry.distance != iv_orig.entry.distance
        || iv.exit.distance != iv_orig.exit.distance;
      if (changed) any_changed = true;

      if (!iv.IsValid() ||
          (changed && iv.Length() < kMinFragmentLength)) {
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

    // Sort surviving residuals by entry distance and rebuild the
    // solution at the nearest one whose Intercept solves; try
    // the next-nearest method if it fails.
    std::sort(residuals.begin(), residuals.begin() + n_res,
              [](const Residual &a, const Residual &b) {
                return a.distance < b.distance;
              });

    bool resolved = false;
    for (std::size_t i = 0; i < n_res; ++i) {
      const auto &r = residuals[i];
      const AirspaceAircraftPerformance perf = PerfFor(
        r.method, glide_polar, cruise_filter,
        circling_filter, circling, task_stats);
      AirspaceInterceptSolution sol =
        w.GetAirspace().Intercept(state, perf,
                                  r.location, r.location);
      if (sol.IsValid() && sol.elapsed_time <= warning_time) {
        w.SetSolution(sol);
        resolved = true;
        break;
      }
    }

    if (!resolved) {
      w.SetCoveredByClearance(true);
    }
  }
}
