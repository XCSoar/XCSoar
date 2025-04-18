// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AlternateTask.hpp"
#include "Task/Points/TaskWaypoint.hpp"
#include "Geo/Math.hpp"
#include "Navigation/Aircraft.hpp"

AlternateTask::AlternateTask(const TaskBehaviour &tb,
                             const Waypoints &wps) noexcept
  :AbortTask(tb, wps)
{
  alternates.reserve(64);
}

void
AlternateTask::Reset() noexcept
{
  AbortTask::Reset();
  destination.SetInvalid();
}

void
AlternateTask::Clear() noexcept
{
  AbortTask::Clear();
  alternates.clear();
}

void 
AlternateTask::ClientUpdate(const AircraftState &state_now,
                            [[maybe_unused]] const bool reachable) noexcept
{
  // build a list of alternates, sorted by distance.
  // this is done in separate stages so we can add the reachable ones
  // before the unreachable ones, without the sort criteria affecting
  // the reachability.

  if (!destination.IsValid())
    return;

  DivertVector q;
  q.reserve(task_points.size());

  const auto straight_distance = state_now.location.Distance(destination);

  for (const auto &i : task_points) {
    const TaskWaypoint &tp = i.point;
    auto wp = tp.GetWaypointPtr();

    const auto diversion_distance =
        ::DoubleDistance(state_now.location, wp->location, destination);
    const auto delta = straight_distance - diversion_distance;

    q.emplace_back(std::move(wp), i.solution, delta);
  }

  /* sort by distance diversion */
  std::sort(q.begin(), q.end(), [](const auto &x, const auto &y){
    return x.delta > y.delta;
  });

  // now push results onto the list, best first.
  const auto n = std::min(q.size(), max_alternates);
  for (std::size_t i = 0; i < n; ++i) {
    AlternatePoint &top = q[i];

    // only add if not already in the list (from previous stage in two
    // stage process)
    if (!IsWaypointInAlternates(*top.waypoint))
      alternates.emplace_back(std::move(top));
  }
}

void
AlternateTask::SetTaskDestination(const GeoPoint &_destination) noexcept
{
  destination = _destination;
}

inline bool
AlternateTask::IsWaypointInAlternates(const Waypoint& waypoint) const noexcept
{
  return std::any_of(alternates.begin(), alternates.end(), [&](const auto &i){
    return *i.waypoint == waypoint;
  });
}
