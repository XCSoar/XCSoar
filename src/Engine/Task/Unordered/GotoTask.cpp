// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GotoTask.hpp"
#include "UnorderedTaskPoint.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Waypoint/Waypoints.hpp"

GotoTask::GotoTask(const TaskBehaviour &tb,
                   const Waypoints &wps)
  :UnorderedTask(TaskType::GOTO, tb),
   waypoints(wps)
{
}

void
GotoTask::SetTaskBehaviour(const TaskBehaviour &tb)
{
  UnorderedTask::SetTaskBehaviour(tb);

  if (tp != NULL)
    tp->SetTaskBehaviour(tb);
}

GotoTask::~GotoTask() 
{
}

TaskWaypoint*
GotoTask::GetActiveTaskPoint() const noexcept
{ 
  return tp.get();
}

bool 
GotoTask::IsValidTaskPoint(const int index_offset) const noexcept
{
  return (index_offset == 0 && tp != NULL);
}


void 
GotoTask::SetActiveTaskPoint([[maybe_unused]] unsigned index) noexcept
{
  // nothing to do
}


bool
GotoTask::UpdateSample([[maybe_unused]] const AircraftState &state,
                       [[maybe_unused]] const GlidePolar &glide_polar,
                       [[maybe_unused]] bool full_update) noexcept
{
  return false; // nothing to do
}


bool 
GotoTask::DoGoto(WaypointPtr &&wp)
{
  if (task_behaviour.goto_nonlandable || wp->IsLandable()) {
    tp = std::make_unique<UnorderedTaskPoint>(std::move(wp), task_behaviour);
    stats.start.Reset();
    force_full_update = true;
    return true;
  } else {
    return false;
  }
}

void
GotoTask::AcceptTaskPointVisitor(TaskPointConstVisitor &visitor) const
{
  if (tp)
    visitor.Visit(*tp);
}

unsigned 
GotoTask::TaskSize() const noexcept
{
  return tp ? 1 : 0;
}

bool 
GotoTask::TakeoffAutotask(const GeoPoint& location, const double terrain_alt)
{
  if (tp)
    return false;

  auto wp = waypoints.GetNearestLandable(location, 5000);
  if (!wp)
    wp = std::make_unique<Waypoint>(waypoints.GenerateTempPoint(location,
                                                                terrain_alt,
                                                              _T("(takeoff)")));

  return DoGoto(std::move(wp));
}
