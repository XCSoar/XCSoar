// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AbortTask.hpp"
#include "AbortIntersectionTest.hpp"
#include "AlternateList.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Solvers/TaskSolution.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Waypoint/Waypoints.hpp"

/** min search range in m */
static constexpr double min_search_range = 50000;

/** max search range in m */
static constexpr double max_search_range = 100000;

AbortTask::AbortTask(const TaskBehaviour &_task_behaviour,
                     const Waypoints &wps) noexcept
  :UnorderedTask(TaskType::ABORT, _task_behaviour),
   waypoints(wps),
   intersection_test(NULL),
   active_waypoint(0)
{
  task_points.reserve(32);
}

void
AbortTask::SetTaskBehaviour(const TaskBehaviour &tb) noexcept
{
  UnorderedTask::SetTaskBehaviour(tb);

  for (auto &tp : task_points)
    tp.point.SetTaskBehaviour(tb);
}

void
AbortTask::SetActiveTaskPoint(unsigned index) noexcept
{
  if (index == active_task_point)
    return;

  if (index < task_points.size()) {
    active_task_point = index;
    active_waypoint = task_points[index].point.GetWaypoint().id;
    stats.start.Reset();
    force_full_update = true;
  }
}

TaskWaypoint *
AbortTask::GetActiveTaskPoint() const noexcept
{
  if (active_task_point < task_points.size())
    // XXX eliminate this deconst hack
    return const_cast<UnorderedTaskPoint *>(&task_points[active_task_point].point);

  return NULL;
}

bool
AbortTask::IsValidTaskPoint(int index_offset) const noexcept
{
  unsigned index = active_task_point + index_offset;
  return (index < task_points.size());
}

unsigned
AbortTask::TaskSize() const noexcept
{
  return task_points.size();
}

void
AbortTask::Clear() noexcept
{
  task_points.clear();
  reachable_landable = false;
}

double
AbortTask::GetAbortRange(const AircraftState &state,
                         const GlidePolar &glide_polar) const noexcept
{
  // always scan at least min range or approx glide range
  return std::clamp(state.altitude * glide_polar.GetBestLD(),
                    min_search_range, max_search_range);
}

[[gnu::pure]]
static bool
IsReachable(const GlideResult &result, bool final_glide) noexcept
{
  return final_glide
    ? result.IsFinalGlide()
    : result.IsAchievable();
}

bool
AbortTask::FillReachable(const AircraftState &state,
                         AlternateList &approx_waypoints,
                         const GlidePolar &polar, bool only_airfield,
                         bool final_glide, [[maybe_unused]] bool safety) noexcept
{
  if (IsTaskFull() || approx_waypoints.empty())
    return false;

  const AGeoPoint p_start(state.location, state.altitude);

  bool found_final_glide = false;
  AlternateList q;
  q.reserve(32);

  for (auto v = approx_waypoints.begin(); v != approx_waypoints.end();) {
    if (only_airfield && !v->waypoint->IsAirport()) {
      ++v;
      continue;
    }

    UnorderedTaskPoint t(v->waypoint, task_behaviour);
    GlideResult result =
        TaskSolution::GlideSolutionRemaining(t, state,
                                             task_behaviour.glide, polar);

    if (IsReachable(result, final_glide)) {
      bool intersects = false;
      const bool is_reachable_final = IsReachable(result, true);

      if (intersection_test && final_glide && is_reachable_final)
        intersects = intersection_test->Intersects(
            AGeoPoint(v->waypoint->location, result.min_arrival_altitude));

      if (!intersects) {
        q.emplace_back(v->waypoint, result);
        // remove it since it's already in the list now      
        v = approx_waypoints.erase(v);

        if (is_reachable_final)
          found_final_glide = true;

        continue; // skip incrementing v since we just erased it
      }
    } 

    ++v;
  }

  /**
   * If dealing with reachable points, sort by arrival altitude.
   * Otherwise, sort by arrival time, which takes into account the wind
   * drift while circling to gain the altitude needed to reach the point.
   */
  if (final_glide) {
    std::sort(q.begin(), q.end(), [](const auto &x, const auto &y){
      return x.solution.altitude_difference > y.solution.altitude_difference;
    });
  } else {
    std::sort(q.begin(), q.end(), [](const auto &x, const auto &y){
      return x.solution.time_elapsed + x.solution.time_virtual <
        y.solution.time_elapsed + y.solution.time_virtual;
    });
  }

  const auto n = std::min(q.size(), max_abort - task_points.size());
  for (std::size_t j = 0; j < n; ++j) {
    auto &top = q[j];
    task_points.emplace_back(std::move(top.waypoint), task_behaviour,
                             top.solution);

    const int i = task_points.size() - 1;
    if (task_points[i].point.GetWaypoint().id == active_waypoint)
      active_task_point = i;
  }

  return found_final_glide;
}

void
AbortTask::ClientUpdate([[maybe_unused]] const AircraftState &state_now,
                        [[maybe_unused]] bool reachable) noexcept
{
  // nothing to do here, it's specialisations that may use this
}

bool
AbortTask::UpdateSample(const AircraftState &state,
                        const GlidePolar &glide_polar,
                        [[maybe_unused]] bool full_update) noexcept
{
  assert(state.location.IsValid());

  Clear();

  unsigned active_waypoint_on_entry;
  if (is_active)
    active_waypoint_on_entry = active_waypoint;
  else {
    active_waypoint = 0;
    active_waypoint_on_entry = (unsigned) -1 ;
  }

  active_task_point = 0; // default to best result if can't find user-set one 

  if (!glide_polar.IsValid())
    /* can't work without a polar */
    return false;

  AlternateList approx_waypoints;
  approx_waypoints.reserve(128);

  waypoints.VisitWithinRange(state.location,
                             GetAbortRange(state, glide_polar), [&approx_waypoints](const auto &wp){
                               if (wp->IsLandable())
                                 approx_waypoints.emplace_back(wp);
                             });
  if (approx_waypoints.empty()) {
    /** @todo increase range */
    return false;
  }

  /**
   * First, get only reachable airfields (no outlanding sites), sort them by
   * arrival altitude, and put them in task_points.
   */
  reachable_landable |=  FillReachable(state, approx_waypoints, glide_polar,
                                       true, true, true);

  /**
   * Now add to task_points reachable outlanding sites, sorted by arrival
   * altitude.
   */
  reachable_landable |=  FillReachable(state, approx_waypoints, glide_polar,
                                       false, true, true);

  /**
   * Add to the "alternates" list the reachable airfield and outlanding site
   * waypoints just added to task_points, sorted according to the "Alternates
   * mode" setting.
   */
  ClientUpdate(state, true);

  /**
   * Finally, add to task_points unreachable landable waypoints (both
   * airfields and outlanding sites, with no preference for either),
   * sorted by arrival time. This sort is done by arrival time instead of
   * by arrival altitude, because for an unreachable point, climbing is
   * required. Because any wind will cause drifting while climbing, the
   * point requiring the least climbing is the one with the earliest
   * arrival time, not necessarily the one with the greatest arrival
   * altitude.
   */
  FillReachable(state, approx_waypoints, glide_polar, false, false, false);

  /**
   * Add to the "alternates" list the unreachable landable waypoints
   * just added to task_points, sorted according to the "Alternates mode"
   * setting.
   */
  ClientUpdate(state, false);

  if (task_points.size()) {
    const TaskWaypoint &task_point = task_points[active_task_point].point;
    active_waypoint = task_point.GetWaypoint().id;
    if (is_active && (active_waypoint_on_entry != active_waypoint)) {
      stats.start.Reset();
      force_full_update = true;
      return true;
    }
  }

  return false; // nothing to do
}

void
AbortTask::AcceptTaskPointVisitor(TaskPointConstVisitor& visitor) const noexcept
{
  for (const auto &tp : task_points)
    visitor.Visit(tp.point);
}

void
AbortTask::Reset() noexcept
{
  Clear();
  UnorderedTask::Reset();
}

WaypointPtr
AbortTask::GetHome() const noexcept
{
  return waypoints.GetHome();
}

GeoVector
AbortTask::GetHomeVector([[maybe_unused]] const AircraftState &state) const noexcept
{
  const auto home_waypoint = GetHome();
  if (home_waypoint)
    return GeoVector(state.location, home_waypoint->location);

  return GeoVector::Invalid();
}
