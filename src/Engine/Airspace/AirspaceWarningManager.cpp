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

static constexpr double CRUISE_FILTER_FACT = 0.5;

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
  notam_day_ack_by_station.clear();
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
  return &warnings.back();
}

bool 
AirspaceWarningManager::Update(const AircraftState& state,
                               const GlidePolar &glide_polar,
                               const TaskStats &task_stats,
                               const bool circling,
                               const std::chrono::duration<unsigned> dt)
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
  bool found = false;
  const double max_alt;
  bool mode_inside = false;

public:
  /**
   * Constructor
   *
   * @param state State of aircraft
   * @param perf Aircraft performance model
   * @param warning_manager Warning manager to add items to
   * @param warning_state Type of warning
   * @param max_time Time limit of intercept
   * @param max_alt Maximum height of base to allow (optional)
   *
   * @return Initialised object
   */
  AirspaceIntersectionWarningVisitor(const AircraftState &_state,
                                     const AirspaceAircraftPerformance &_perf,
                                     AirspaceWarningManager &_warning_manager,
                                     const AirspaceWarning::State _warning_state,
                                     const FloatDuration _max_time,
                                     const double _max_alt = -1):
    state(_state),
    perf(_perf),
    warning_manager(_warning_manager),
    warning_state(_warning_state),
    max_time(_max_time),
    max_alt(_max_alt)
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
        return; // ignore inactive airspaces completely

      if (!(warning_manager.GetConfig().IsClassEnabled(airspace.GetClassOrType()) ||
            warning_manager.GetConfig().IsClassEnabled(airspace.GetTypeOrClass())) ||
          ExcludeAltitude(airspace))
        return;

      AirspaceWarning *warning = warning_manager.GetWarningPtr(airspace);
      if (warning == nullptr || warning->IsStateAccepted(warning_state)) {

        AirspaceInterceptSolution solution;

        if (mode_inside) {
          solution = airspace.Intercept(state, perf,
                                        state.location, state.location);
        } else {
          solution = Intercept(airspace, state, perf);
        }
        if (!solution.IsValid())
          return;
        if (solution.elapsed_time > max_time)
          return;

        if (warning == nullptr)
          warning = warning_manager.GetNewWarningPtr(std::move(airspace_ptr));

        warning->UpdateSolution(warning_state, solution);
        found = true;
      }
    } catch (const std::exception &e) {
#ifndef NDEBUG
      PrintException(e);
#else
      (void)e;
#endif
    } catch (...) {
#ifndef NDEBUG
      PrintException(std::current_exception());
#endif
    }
  }

  void Visit(ConstAirspacePtr as) noexcept override {
    Intersection(as);
  }

  /**
   * Determine whether intersections for this type were found (new or modified)
   *
   * @return True if intersections were found
   */
  bool Found() const {
    return found;
  }

  void SetMode(bool m) {
    mode_inside = m;
  }

private:
  bool ExcludeAltitude(const AbstractAirspace& airspace) {
    if (max_alt <= 0)
      return false;

    return (airspace.GetBaseAltitude(state) > max_alt);
  }
};


bool 
AirspaceWarningManager::UpdatePredicted(const AircraftState& state, 
                                        const GeoPoint &location_predicted,
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

  AirspaceIntersectionWarningVisitor visitor(state, perf, 
                                             *this, 
                                             warning_state, max_time_limit,
                                             ceiling);

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

  return UpdatePredicted(state, location_tp, perf_task,
                          AirspaceWarning::WARNING_TASK, time_remaining);
}


bool 
AirspaceWarningManager::UpdateFilter(const AircraftState& state, const bool circling)
{
  // update both filters even though we are using only one
  cruise_filter.Update(state);
  circling_filter.Update(state);

  const GeoPoint location_predicted = circling?
    circling_filter.GetPredictedState(prediction_time_filter).location:
    cruise_filter.GetPredictedState(prediction_time_filter).location;

  if (circling) 
    return UpdatePredicted(state, location_predicted,
                           AirspaceAircraftPerformance(circling_filter),
                            AirspaceWarning::WARNING_FILTER, prediction_time_filter);
  else
    return UpdatePredicted(state, location_predicted,
                           AirspaceAircraftPerformance(cruise_filter),
                            AirspaceWarning::WARNING_FILTER, prediction_time_filter);
}


bool 
AirspaceWarningManager::UpdateGlide(const AircraftState &state,
                                    const GlidePolar &glide_polar)
{
  if (!glide_polar.IsValid())
    return false;

  const GeoPoint location_predicted = 
    state.GetPredictedState(prediction_time_glide).location;

  const AirspaceAircraftPerformance perf_glide(glide_polar);
  return UpdatePredicted(state, location_predicted,
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
  if (warning->IsWarningAcknowledged() != was_acknowledged)
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
  if (warning->IsInsideAcknowledged() != was_acknowledged)
    ++serial;
}

void
AirspaceWarningManager::AcknowledgeDay(ConstAirspacePtr airspace,
                                       const bool set)
{
  const char *const key = NotamDayAckKey(*airspace);
  const bool was_member = key != nullptr &&
    notam_day_ack_by_station.contains(key);

  AirspaceWarning *warning = nullptr;
  if (set) {
    warning = &GetWarning(std::move(airspace));
  } else {
    // Avoid creating a warning when just clearing an acknowledgement.
    warning = GetWarningPtr(*airspace);
    if (warning == nullptr && key != nullptr)
      warning = FindWarningByNotamDayAckKey(key);
    if (warning == nullptr && !was_member)
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

  if (warning != nullptr)
    warning->AcknowledgeDay(set);

  const bool is_acknowledged = warning != nullptr && warning->GetAckDay();
  if (membership_changed || is_acknowledged != was_acknowledged)
    ++serial;
}

bool
AirspaceWarningManager::GetAckDay(const AbstractAirspace &airspace) const noexcept
{
  if (const char *key = NotamDayAckKey(airspace);
      key != nullptr &&
      notam_day_ack_by_station.contains(key))
    return true;

  const AirspaceWarning *warning = GetWarningPtr(airspace);
  return warning != nullptr && warning->GetAckDay();
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
