/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "OrderedTask.hpp"
#include "Task/TaskEvents.hpp"
#include "TaskAdvance.hpp"
#include "Points/OrderedTaskPoint.hpp"
#include "Points/StartPoint.hpp"
#include "Points/FinishPoint.hpp"
#include "Task/Solvers/TaskMacCreadyTravelled.hpp"
#include "Task/Solvers/TaskMacCreadyRemaining.hpp"
#include "Task/Solvers/TaskMacCreadyTotal.hpp"
#include "Task/Solvers/TaskCruiseEfficiency.hpp"
#include "Task/Solvers/TaskEffectiveMacCready.hpp"
#include "Task/Solvers/TaskBestMc.hpp"
#include "Task/Solvers/TaskMinTarget.hpp"
#include "Task/Solvers/TaskGlideRequired.hpp"
#include "Task/Solvers/TaskOptTarget.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"

#include "Task/Factory/Create.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/Factory/TaskFactoryConstraints.hpp"
#include "Task/ObservationZones/MatCylinderZone.hpp"

#include "Waypoint/Waypoints.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "Task/Stats/TaskSummary.hpp"
#include "Task/PathSolvers/TaskDijkstraMin.hpp"
#include "Task/PathSolvers/TaskDijkstraMax.hpp"

OrderedTask::OrderedTask(const TaskBehaviour &tb)
  :AbstractTask(TaskType::ORDERED, tb),
   taskpoint_start(NULL),
   taskpoint_finish(NULL),
   factory_mode(tb.task_type_default),
   active_factory(NULL),
   ordered_behaviour(tb.ordered_defaults),
   dijkstra_min(NULL), dijkstra_max(NULL)
{
  active_factory = CreateTaskFactory(factory_mode, *this, task_behaviour);
  active_factory->UpdateOrderedTaskBehaviour(ordered_behaviour);
  task_advance.SetFactoryConstraints(active_factory->GetConstraints());
}

OrderedTask::~OrderedTask()
{
  RemoveAllPoints();
  ClearMatPoints();

  delete active_factory;

  delete dijkstra_min;
  delete dijkstra_max;
}

const TaskFactoryConstraints &
OrderedTask::GetFactoryConstraints() const
{
  return GetFactory().GetConstraints();
}

static void
SetTaskBehaviour(OrderedTask::OrderedTaskPointVector &vector,
                 const TaskBehaviour &tb)
{
  for (auto i : vector)
    i->SetTaskBehaviour(tb);
}

void
OrderedTask::SetTaskBehaviour(const TaskBehaviour &tb)
{
  AbstractTask::SetTaskBehaviour(tb);

  ::SetTaskBehaviour(task_points, tb);
  ::SetTaskBehaviour(optional_start_points, tb);
}

static void
UpdateObservationZones(OrderedTask::OrderedTaskPointVector &points,
                       const TaskProjection &task_projection)
{
  for (auto i : points)
    i->UpdateOZ(task_projection);
}

void
OrderedTask::UpdateGeometry()
{
  ScanStartFinish();

  if (!HasStart() || !task_points[0])
    return;

  taskpoint_start->ScanActive(*task_points[active_task_point]);

  // scan location of task points
  task_projection.Reset(task_points[0]->GetLocation());
  for (const auto *tp : task_points)
    tp->ScanProjection(task_projection);

  // scan location of mat control points
  for (const auto *i : GetMatPoints())
    i->ScanProjection(task_projection);

  // ... and optional start points
  for (const OrderedTaskPoint *tp : optional_start_points)
    tp->ScanProjection(task_projection);

  // projection can now be determined
  task_projection.Update();

  // update OZ's for items that depend on next-point geometry 
  UpdateObservationZones(task_points, task_projection);
  UpdateObservationZones(SetMatPoints(), task_projection);
  UpdateObservationZones(optional_start_points, task_projection);

  // now that the task projection is stable, and oz is stable,
  // calculate the bounding box in projected coordinates
  for (const auto tp : GetMatPoints())
    tp->UpdateBoundingBox(task_projection);

  for (const auto tp : task_points)
    tp->UpdateBoundingBox(task_projection);

  for (const auto tp : optional_start_points)
    tp->UpdateBoundingBox(task_projection);

  // update stats so data can be used during task construction
  /// @todo this should only be done if not flying! (currently done with has_entered)
  if (!taskpoint_start->HasEntered()) {
    GeoPoint loc = taskpoint_start->GetLocation();
    UpdateStatsDistances(loc, true);
    if (HasFinish()) {
      /// @todo: call AbstractTask::update stats methods with fake state
      /// so stats are updated
    }
  }
}

// TIMES

fixed 
OrderedTask::ScanTotalStartTime(const AircraftState &)
{
  if (taskpoint_start)
    return taskpoint_start->GetEnteredState().time;

  return fixed(0);
}

fixed 
OrderedTask::ScanLegStartTime(const AircraftState &)
{
  if (active_task_point)
    return task_points[active_task_point-1]->GetEnteredState().time;

  return fixed(-1);
}

// DISTANCES

inline fixed
OrderedTask::ScanDistanceMin(const GeoPoint &location, bool full)
{
  if (full) {
    if (dijkstra_min == NULL)
      dijkstra_min = new TaskDijkstraMin();

    SearchPoint ac(location, task_projection);
    if (dijkstra_min->DistanceMin(*this, ac)) {
      for (unsigned i = GetActiveIndex(), end = TaskSize(); i != end; ++i)
        SetPointSearchMin(i, dijkstra_min->GetSolution(i));
    }

    last_min_location = location;
  }

  return taskpoint_start->ScanDistanceMin();
}

inline fixed
OrderedTask::ScanDistanceMax()
{
  if (task_points.empty()) // nothing to do!
    return fixed(0);

  assert(active_task_point < task_points.size());

  // for max calculations, since one can still travel further in the
  // sector, we pretend we are on the previous turnpoint so the
  // search samples will contain the full boundary
  const unsigned atp = active_task_point;
  if (atp) {
    active_task_point--;
    taskpoint_start->ScanActive(*task_points[active_task_point]);
  }

  if (dijkstra_max == NULL)
    dijkstra_max = new TaskDijkstraMax();

  if (dijkstra_max->DistanceMax(*this)) {
    for (unsigned i = 0, active = GetActiveIndex(), end = TaskSize();
         i != end; ++i) {
      const SearchPoint &solution = dijkstra_max->GetSolution(i);
      SetPointSearchMax(i, solution);
      if (i <= active)
        set_tp_search_achieved(i, solution);
    }
  }

  if (atp) {
    active_task_point = atp;
    taskpoint_start->ScanActive(*task_points[active_task_point]);
  }
  return taskpoint_start->ScanDistanceMax();
}

void
OrderedTask::ScanDistanceMinMax(const GeoPoint &location, bool force,
                                fixed *dmin, fixed *dmax)
{
  if (!taskpoint_start)
    return;

  if (force)
    *dmax = ScanDistanceMax();

  bool force_min = force || DistanceIsSignificant(location, last_min_location);
  *dmin = ScanDistanceMin(location, force_min);
}

fixed
OrderedTask::ScanDistanceNominal()
{
  if (taskpoint_start)
    return taskpoint_start->ScanDistanceNominal();

  return fixed(0);
}

fixed
OrderedTask::ScanDistanceScored(const GeoPoint &location)
{
  if (taskpoint_start)
    return taskpoint_start->ScanDistanceScored(location);

  return fixed(0);
}

fixed
OrderedTask::ScanDistanceRemaining(const GeoPoint &location)
{
  if (taskpoint_start)
    return taskpoint_start->ScanDistanceRemaining(location);

  return fixed(0);
}

fixed
OrderedTask::ScanDistanceTravelled(const GeoPoint &location)
{
  if (taskpoint_start)
    return taskpoint_start->ScanDistanceTravelled(location);

  return fixed(0);
}

fixed
OrderedTask::ScanDistancePlanned()
{
  if (taskpoint_start)
    return taskpoint_start->ScanDistancePlanned();

  return fixed(0);
}

unsigned
OrderedTask::GetLastIntermediateAchieved() const
{
  if (TaskSize() < 2)
    return 0;

  for (unsigned i = 1; i < TaskSize() - 1; i++)
    if (!task_points[i]->HasEntered())
      return i - 1;
  return TaskSize() - 2;
}

bool
OrderedTask::ShouldAddToMat(const Waypoint &mat_wp) const
{
  unsigned last_achieved_index = GetLastIntermediateAchieved();

  // is this the same point we just achieved? then ignore it
  if (mat_wp == GetPoint(last_achieved_index).GetWaypoint())
  {
    return false;
  }

  // The TP after the last achieved is already next in the task
  // and it is not the finish so do nothing
  if ((last_achieved_index + 1 < TaskSize() -1)
    && mat_wp == GetPoint(last_achieved_index + 1).GetWaypoint()) {
    return false;
  }
  return true;
}

// TRANSITIONS

bool 
OrderedTask::CheckTransitions(const AircraftState &state,
                              const AircraftState &state_last)
{
  if (!taskpoint_start)
    return false;

  taskpoint_start->ScanActive(*task_points[active_task_point]);

  if (!state.flying)
    return false;

  const int n_task = task_points.size();

  if (!n_task)
    return false;

  FlatBoundingBox bb_last(task_projection.ProjectInteger(state_last.location),
                          1);
  FlatBoundingBox bb_now(task_projection.ProjectInteger(state.location),
                         1);

  bool last_started = TaskStarted();
  const bool last_finished = TaskFinished();

  const int t_min = std::max(0, (int)active_task_point - 1);
  const int t_max = std::min(n_task - 1, (int)active_task_point);
  bool full_update = false;

  for (int i = t_min; i <= t_max; i++) {

    bool transition_enter = false;
    bool transition_exit = false;

    if (i==0) {
      full_update |= CheckTransitionOptionalStart(state, state_last,
                                                  bb_now, bb_last,
                                                  transition_enter,
                                                  transition_exit,
                                                  last_started);
    }

    full_update |= CheckTransitionPoint(*task_points[i],
                                        state, state_last, bb_now, bb_last,
                                        transition_enter, transition_exit,
                                        last_started, i == 0);

    if (i == (int)active_task_point) {
      const bool last_request_armed = task_advance.NeedToArm();

      if (task_advance.CheckReadyToAdvance(*task_points[i], state,
                                           transition_enter,
                                           transition_exit)) {
        task_advance.SetArmed(false);

        if (i + 1 < n_task) {
          i++;
          SetActiveTaskPoint(i);
          taskpoint_start->ScanActive(*task_points[active_task_point]);

          if (task_events != NULL)
            task_events->ActiveAdvanced(*task_points[i], i);

          // on sector exit, must update samples since start sector
          // exit transition clears samples
          full_update = true;
        }
      } else if (!last_request_armed && task_advance.NeedToArm()) {
        if (task_events != NULL)
          task_events->RequestArm(*task_points[i]);
      }
    }
  }

  taskpoint_start->ScanActive(*task_points[active_task_point]);

  stats.task_finished = TaskFinished();
  stats.task_started = TaskStarted();

  if (stats.task_started)
    taskpoint_finish->set_fai_finish_height(GetStartState().altitude - fixed(1000));

  if (task_events != NULL) {
    if (stats.task_started && !last_started)
      task_events->TaskStart();

    if (stats.task_finished && !last_finished)
      task_events->TaskFinish();
  }

  return full_update;
}

inline bool
OrderedTask::CheckTransitionOptionalStart(const AircraftState &state,
                                          const AircraftState &state_last,
                                          const FlatBoundingBox& bb_now,
                                          const FlatBoundingBox& bb_last,
                                          bool &transition_enter,
                                          bool &transition_exit,
                                          bool &last_started)
{
  bool full_update = false;

  for (auto begin = optional_start_points.cbegin(),
         end = optional_start_points.cend(), i = begin; i != end; ++i) {
    full_update |= CheckTransitionPoint(**i,
                                        state, state_last, bb_now, bb_last,
                                        transition_enter, transition_exit,
                                        last_started, true);

    if (transition_enter || transition_exit) {
      // we have entered or exited this optional start point, so select it.
      // user has no choice in this: rules for multiple start points are that
      // the last start OZ flown through is used for scoring

      SelectOptionalStart(std::distance(begin, i));

      return full_update;
    }
  }
  return full_update;
}

bool
OrderedTask::CheckTransitionPointMat(OrderedTaskPoint &point,
                                     const AircraftState &state,
                                     const AircraftState &state_last,
                                     const FlatBoundingBox &bb_now,
                                     const FlatBoundingBox &bb_last)
{
  const bool nearby = point.BoundingBoxOverlaps(bb_now);

  return nearby && point.CheckEnterTransitionMat(state, state_last);
}

bool
OrderedTask::CheckTransitionPoint(OrderedTaskPoint &point,
                                  const AircraftState &state,
                                  const AircraftState &state_last,
                                  const FlatBoundingBox &bb_now,
                                  const FlatBoundingBox &bb_last,
                                  bool &transition_enter,
                                  bool &transition_exit,
                                  bool &last_started,
                                  const bool is_start)
{
  const bool nearby = point.BoundingBoxOverlaps(bb_now) ||
    point.BoundingBoxOverlaps(bb_last);

  if (nearby && point.TransitionEnter(state, state_last)) {
    transition_enter = true;

    if (task_events != NULL)
      task_events->EnterTransition(point);
  }
  
  if (nearby && point.TransitionExit(state, state_last, task_projection)) {
    transition_exit = true;

    if (task_events != NULL)
      task_events->ExitTransition(point);
    
    // detect restart
    if (is_start && last_started)
      last_started = false;
  }
  
  if (is_start) 
    UpdateStartTransition(state, point);
  
  return nearby
    ? point.UpdateSampleNear(state, task_projection)
    : point.UpdateSampleFar(state, task_projection);
}

// ADDITIONAL FUNCTIONS

bool 
OrderedTask::UpdateIdle(const AircraftState &state,
                        const GlidePolar &glide_polar)
{
  bool retval = AbstractTask::UpdateIdle(state, glide_polar);

  if (HasStart() && task_behaviour.optimise_targets_range &&
      positive(GetOrderedTaskBehaviour().aat_min_time)) {

    CalcMinTarget(state, glide_polar,
                  GetOrderedTaskBehaviour().aat_min_time + fixed(task_behaviour.optimise_targets_margin));

    if (task_behaviour.optimise_targets_bearing &&
        task_points[active_task_point]->GetType() == TaskPointType::AAT) {
      AATPoint *ap = (AATPoint *)task_points[active_task_point];
      // very nasty hack
      TaskOptTarget tot(task_points, active_task_point, state,
                        task_behaviour.glide, glide_polar,
                        *ap, task_projection, taskpoint_start);
      tot.search(fixed(0.5));
    }
    retval = true;
  }
  
  return retval;
}

bool 
OrderedTask::UpdateSample(const AircraftState &state,
                          const GlidePolar &glide_polar,
                           const bool full_update)
{
  stats.inside_oz = active_task_point < task_points.size() &&
    task_points[active_task_point]->IsInSector(state);

  return true;
}

// TASK

void
OrderedTask::SetNeighbours(unsigned position)
{
  OrderedTaskPoint* prev = NULL;
  OrderedTaskPoint* next = NULL;

  if (!task_points[position])
    // nothing to do if this is deleted
    return;

  if (position >= task_points.size())
    // nothing to do
    return;

  if (position > 0)
    prev = task_points[position - 1];

  if (position + 1 < task_points.size())
    next = task_points[position + 1];

  task_points[position]->SetNeighbours(prev, next);

  if (position==0) {
    for (const auto tp : optional_start_points)
      tp->SetNeighbours(prev, next);
  }
}

bool
OrderedTask::CheckTask() const
{
  return this->GetFactory().Validate();
}

AATPoint*
OrderedTask::GetAATTaskPoint(unsigned TPindex) const
{
 if (TPindex > task_points.size() - 1) {
   return NULL;
 }
 if (task_points[TPindex]) {
    if (task_points[TPindex]->GetType() == TaskPointType::AAT)
      return (AATPoint*) task_points[TPindex];
    else
      return (AATPoint*)NULL;
 }
 return NULL;
}

bool
OrderedTask::ScanStartFinish()
{
  /// @todo also check there are not more than one start/finish point
  if (task_points.empty()) {
    taskpoint_start = NULL;
    taskpoint_finish = NULL;
    return false;
  }

  taskpoint_start = task_points[0]->GetType() == TaskPointType::START
    ? (StartPoint *)task_points[0]
    : NULL;

  taskpoint_finish = task_points.size() > 1 &&
    task_points[task_points.size() - 1]->GetType() == TaskPointType::FINISH
    ? (FinishPoint *)task_points[task_points.size() - 1]
    : NULL;

  return HasStart() && HasFinish();
}

inline void
OrderedTask::ErasePoint(const unsigned index)
{
  delete task_points[index];
  task_points.erase(task_points.begin() + index);
}

inline void
OrderedTask::EraseOptionalStartPoint(const unsigned index)
{
  delete optional_start_points[index];
  optional_start_points.erase(optional_start_points.begin() + index);
}

bool
OrderedTask::Remove(const unsigned position)
{
  if (position >= task_points.size())
    return false;

  if (active_task_point > position ||
      (active_task_point > 0 && active_task_point == task_points.size() - 1))
    active_task_point--;

  ErasePoint(position);

  SetNeighbours(position);
  if (position)
    SetNeighbours(position - 1);

  UpdateGeometry();
  return true;
}

bool
OrderedTask::RemoveOptionalStart(const unsigned position)
{
  if (position >= optional_start_points.size())
    return false;

  EraseOptionalStartPoint(position);

  if (task_points.size()>1)
    SetNeighbours(0);

  UpdateGeometry();
  return true;
}

bool 
OrderedTask::Append(const OrderedTaskPoint &new_tp)
{
  if (/* is the new_tp allowed in this context? */
      (!task_points.empty() && !new_tp.IsPredecessorAllowed()) ||
      /* can a tp be appended after the last one? */
      (task_points.size() >= 1 &&
       !task_points[task_points.size() - 1]->IsSuccessorAllowed()))
    return false;

  task_points.push_back(new_tp.Clone(task_behaviour, ordered_behaviour));
  if (task_points.size() > 1)
    SetNeighbours(task_points.size() - 2);
  else {
    // give it a value when we have one tp so it is not uninitialised
    last_min_location = new_tp.GetLocation();
  }

  SetNeighbours(task_points.size() - 1);
  UpdateGeometry();
  return true;
}

bool 
OrderedTask::AppendOptionalStart(const OrderedTaskPoint &new_tp)
{
  optional_start_points.push_back(new_tp.Clone(task_behaviour,
                                               ordered_behaviour));
  if (task_points.size() > 1)
    SetNeighbours(0);
  UpdateGeometry();
  return true;
}

bool 
OrderedTask::Insert(const OrderedTaskPoint &new_tp, const unsigned position)
{
  if (position >= task_points.size())
    return Append(new_tp);

  if (/* is the new_tp allowed in this context? */
      (position > 0 && !new_tp.IsPredecessorAllowed()) ||
      !new_tp.IsSuccessorAllowed() ||
      /* can a tp be inserted at this position? */
      (position > 0 && !task_points[position - 1]->IsSuccessorAllowed()) ||
      !task_points[position]->IsPredecessorAllowed())
    return false;

  if (active_task_point >= position)
    active_task_point++;

  task_points.insert(task_points.begin() + position,
                     new_tp.Clone(task_behaviour, ordered_behaviour));

  if (position)
    SetNeighbours(position - 1);

  SetNeighbours(position);
  SetNeighbours(position + 1);

  UpdateGeometry();
  return true;
}

bool 
OrderedTask::Replace(const OrderedTaskPoint &new_tp, const unsigned position)
{
  if (position >= task_points.size())
    return false;

  if (task_points[position]->Equals(new_tp))
    // nothing to do
    return true;

  /* is the new_tp allowed in this context? */
  if ((position > 0 && !new_tp.IsPredecessorAllowed()) ||
      (position + 1 < task_points.size() && !new_tp.IsSuccessorAllowed()))
    return false;

  delete task_points[position];
  task_points[position] = new_tp.Clone(task_behaviour, ordered_behaviour);

  if (position)
    SetNeighbours(position - 1);

  SetNeighbours(position);
  if (position + 1 < task_points.size())
    SetNeighbours(position + 1);

  UpdateGeometry();
  return true;
}


bool 
OrderedTask::ReplaceOptionalStart(const OrderedTaskPoint &new_tp,
                                  const unsigned position)
{
  if (position >= optional_start_points.size())
    return false;

  if (optional_start_points[position]->Equals(new_tp))
    // nothing to do
    return true;

  delete optional_start_points[position];
  optional_start_points[position] = new_tp.Clone(task_behaviour,
                                                 ordered_behaviour);

  SetNeighbours(0);
  UpdateGeometry();
  return true;
}


void 
OrderedTask::SetActiveTaskPoint(unsigned index)
{
  if (index < task_points.size()) {
    if (active_task_point != index)
      task_advance.SetArmed(false);

    active_task_point = index;
  } else if (task_points.empty()) {
    active_task_point = 0;
  }
}

TaskWaypoint*
OrderedTask::GetActiveTaskPoint() const
{
  if (active_task_point < task_points.size())
    return task_points[active_task_point];

  return NULL;
}

bool 
OrderedTask::IsValidTaskPoint(const int index_offset) const
{
  unsigned index = active_task_point + index_offset;
  return (index < task_points.size());
}


void
OrderedTask::GlideSolutionRemaining(const AircraftState &aircraft,
                                      const GlidePolar &polar,
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyRemaining tm(task_points.cbegin(), task_points.cend(),
                            active_task_point,
                            task_behaviour.glide, polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution();
}

void
OrderedTask::GlideSolutionTravelled(const AircraftState &aircraft,
                                    const GlidePolar &glide_polar,
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyTravelled tm(task_points.cbegin(), active_task_point,
                            task_behaviour.glide, glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution();
}

void
OrderedTask::GlideSolutionPlanned(const AircraftState &aircraft,
                                  const GlidePolar &glide_polar,
                                    GlideResult &total,
                                    GlideResult &leg,
                                    DistanceStat &total_remaining_effective,
                                    DistanceStat &leg_remaining_effective,
                                    const GlideResult &solution_remaining_total,
                                    const GlideResult &solution_remaining_leg)
{
  TaskMacCreadyTotal tm(task_points.cbegin(), task_points.cend(),
                        active_task_point,
                        task_behaviour.glide, glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution();

  if (solution_remaining_total.IsOk())
    total_remaining_effective.SetDistance(tm.effective_distance(solution_remaining_total.time_elapsed));
  else
    total_remaining_effective.Reset();

  if (solution_remaining_leg.IsOk())
    leg_remaining_effective.SetDistance(tm.effective_leg_distance(solution_remaining_leg.time_elapsed));
  else
    leg_remaining_effective.Reset();
}

// Auxiliary glide functions

fixed
OrderedTask::CalcRequiredGlide(const AircraftState &aircraft,
                               const GlidePolar &glide_polar) const
{
  TaskGlideRequired bgr(task_points, active_task_point, aircraft,
                        task_behaviour.glide, glide_polar);
  return bgr.search(fixed(0));
}

bool
OrderedTask::CalcBestMC(const AircraftState &aircraft,
                        const GlidePolar &glide_polar,
                        fixed &best) const
{
  // note setting of lower limit on mc
  TaskBestMc bmc(task_points, active_task_point, aircraft,
                 task_behaviour.glide, glide_polar);
  return bmc.search(glide_polar.GetMC(), best);
}


bool
OrderedTask::AllowIncrementalBoundaryStats(const AircraftState &aircraft) const
{
  if (!active_task_point)
    return false;
  assert(task_points[active_task_point]);

  if (task_points[active_task_point]->IsBoundaryScored())
    return true;

  bool in_sector = task_points[active_task_point]->IsInSector(aircraft);
  if (active_task_point>0) {
    in_sector |= task_points[active_task_point-1]->IsInSector(aircraft);
  }
  return !in_sector;
}

bool
OrderedTask::CalcCruiseEfficiency(const AircraftState &aircraft,
                                  const GlidePolar &glide_polar,
                                  fixed &val) const
{
  if (AllowIncrementalBoundaryStats(aircraft)) {
    TaskCruiseEfficiency bce(task_points, active_task_point, aircraft,
                             task_behaviour.glide, glide_polar);
    val = bce.search(fixed(1));
    return true;
  } else {
    val = fixed(1);
    return false;
  }
}

bool 
OrderedTask::CalcEffectiveMC(const AircraftState &aircraft,
                             const GlidePolar &glide_polar,
                             fixed &val) const
{
  if (AllowIncrementalBoundaryStats(aircraft)) {
    TaskEffectiveMacCready bce(task_points, active_task_point, aircraft,
                               task_behaviour.glide, glide_polar);
    val = bce.search(glide_polar.GetMC());
    return true;
  } else {
    val = glide_polar.GetMC();
    return false;
  }
}


inline fixed
OrderedTask::CalcMinTarget(const AircraftState &aircraft,
                           const GlidePolar &glide_polar,
                           const fixed t_target)
{
  if (stats.distance_max > stats.distance_min) {
    // only perform scan if modification is possible
    const fixed t_rem = std::max(fixed(0),
                                 t_target - stats.total.time_elapsed);

    TaskMinTarget bmt(task_points, active_task_point, aircraft,
                      task_behaviour.glide, glide_polar,
                      t_rem, taskpoint_start);
    fixed p = bmt.search(fixed(0));
    return p;
  }

  return fixed(0);
}


fixed 
OrderedTask::CalcGradient(const AircraftState &state) const
{
  if (task_points.empty())
    return fixed(0);

  // Iterate through remaining turnpoints
  fixed distance = fixed(0);
  for (const OrderedTaskPoint *tp : task_points)
    // Sum up the leg distances
    distance += tp->GetVectorRemaining(state.location).distance;

  if (!positive(distance))
    return fixed(0);

  // Calculate gradient to the last turnpoint of the remaining task
  return (state.altitude - task_points[task_points.size() - 1]->GetElevation()) / distance;
}

static void
Visit(const OrderedTask::OrderedTaskPointVector &points,
      TaskPointConstVisitor &visitor)
{
  for (const TaskPoint *tp : points)
    visitor.Visit(*tp);
}

void 
OrderedTask::AcceptTaskPointVisitor(TaskPointConstVisitor& visitor) const
{
  Visit(task_points, visitor);
}

static void
ResetPoints(OrderedTask::OrderedTaskPointVector &points)
{
  for (auto *i : points)
    i->Reset();
}

void
OrderedTask::Reset()
{  
  /// @todo also reset data in this class e.g. stats?
  ResetPoints(task_points);
  ResetPoints(optional_start_points);

  AbstractTask::Reset();
  stats.task_finished = false;
  stats.task_started = false;
  task_advance.Reset();
  SetActiveTaskPoint(0);
}

bool 
OrderedTask::TaskFinished() const
{
  if (taskpoint_finish)
    return (taskpoint_finish->HasEntered());

  return false;
}

bool 
OrderedTask::TaskStarted(bool soft) const
{
  if (taskpoint_start) {
    // have we really started?
    if (taskpoint_start->HasExited()) 
      return true;

    // if soft starts allowed, consider started if we progressed to next tp
    if (soft && (active_task_point>0))
      return true;
  }

  return false;
}

/**
 * Test whether two points (as previous search locations) are significantly
 * different to warrant a new search
 *
 * @param a1 First point to compare
 * @param a2 Second point to compare
 * @param dist_threshold Threshold distance for significance
 *
 * @return True if distance is significant
 */
gcc_pure
static bool
DistanceIsSignificant(const SearchPoint &a1, const SearchPoint &a2,
                      const unsigned dist_threshold = 1)
{
  return a1.FlatSquareDistanceTo(a2) > (dist_threshold * dist_threshold);
}

inline bool
OrderedTask::DistanceIsSignificant(const GeoPoint &location,
                                   const GeoPoint &location_last) const
{
  SearchPoint a1(location, task_projection);
  SearchPoint a2(location_last, task_projection);
  return ::DistanceIsSignificant(a1, a2);
}


const SearchPointVector &
OrderedTask::GetPointSearchPoints(unsigned tp) const
{
  return task_points[tp]->GetSearchPoints();
}

void 
OrderedTask::SetPointSearchMin(unsigned tp, const SearchPoint &sol)
{
  if (!tp && !task_points[0]->HasExited())
    return;

  task_points[tp]->SetSearchMin(sol);
}

void 
OrderedTask::set_tp_search_achieved(unsigned tp, const SearchPoint &sol) 
{
  if (task_points[tp]->HasSampled())
    SetPointSearchMin(tp, sol);
}

void 
OrderedTask::SetPointSearchMax(unsigned tp, const SearchPoint &sol)
{
  task_points[tp]->SetSearchMax(sol);
}

bool
OrderedTask::IsFull() const
{
  return TaskSize() == GetFactory().GetConstraints().max_points;
}

inline void
OrderedTask::UpdateStartTransition(const AircraftState &state,
                                   OrderedTaskPoint &start)
{
  if (active_task_point == 0) {
    // find boundary point that produces shortest
    // distance from state to that point to next tp point
    taskpoint_start->find_best_start(state, *task_points[1], task_projection);
  } else if (!start.HasExited() && !start.IsInSector(state)) {
    start.Reset();
    // reset on invalid transition to outside
    // point to nominal start point
  }
  // @todo: modify this for optional start?
}

AircraftState 
OrderedTask::GetStartState() const
{
  if (HasStart() && TaskStarted())
    return taskpoint_start->GetEnteredState();

  // @todo: modify this for optional start?

  AircraftState null_state;
  return null_state;
}

AircraftState 
OrderedTask::GetFinishState() const
{
  if (HasFinish() && TaskFinished())
    return taskpoint_finish->GetEnteredState();

  AircraftState null_state;
  return null_state;
}

bool
OrderedTask::HasTargets() const
{
  for (const OrderedTaskPoint *tp : task_points)
    if (tp->HasTarget())
      return true;

  return false;
}

fixed
OrderedTask::GetFinishHeight() const
{
  if (taskpoint_finish)
    return taskpoint_finish->GetElevation();

  return fixed(0);
}

GeoPoint 
OrderedTask::GetTaskCenter() const
{
  if (!HasStart() || !task_points[0])
    return GeoPoint::Invalid();

  return task_projection.GetCenter();
}

fixed 
OrderedTask::GetTaskRadius() const
{ 
  if (!HasStart() || !task_points[0])
    return fixed(0);

  return task_projection.ApproxRadius();
}

OrderedTask* 
OrderedTask::Clone(const TaskBehaviour &tb) const
{
  OrderedTask* new_task = new OrderedTask(tb);

  new_task->ordered_behaviour = ordered_behaviour;

  new_task->SetFactory(factory_mode);
  for (const OrderedTaskPoint *tp : task_points)
    new_task->Append(*tp);

  for (const OrderedTaskPoint *tp : optional_start_points)
    new_task->AppendOptionalStart(*tp);

  new_task->active_task_point = active_task_point;
  new_task->UpdateGeometry();
  return new_task;
}

void
OrderedTask::CheckDuplicateWaypoints(Waypoints& waypoints,
                                     OrderedTaskPointVector& points,
                                     const bool is_task)
{
  for (auto begin = points.cbegin(), end = points.cend(), i = begin;
       i != end; ++i) {
    const Waypoint &wp =
      waypoints.CheckExistsOrAppend((*i)->GetWaypoint());

    const OrderedTaskPoint *new_tp =
      (*i)->Clone(task_behaviour, ordered_behaviour, &wp);
    if (is_task)
      Replace(*new_tp, std::distance(begin, i));
    else
      ReplaceOptionalStart(*new_tp, std::distance(begin, i));
    delete new_tp;
  }
}

void
OrderedTask::CheckDuplicateWaypoints(Waypoints& waypoints)
{
  CheckDuplicateWaypoints(waypoints, task_points, true);
  CheckDuplicateWaypoints(waypoints, optional_start_points, false);
}

bool
OrderedTask::Commit(const OrderedTask& that)
{
  bool modified = false;

  // change mode to that one 
  SetFactory(that.factory_mode);

  // copy across behaviour
  ordered_behaviour = that.ordered_behaviour;

  // remove if that task is smaller than this one
  while (TaskSize() > that.TaskSize()) {
    Remove(TaskSize() - 1);
    modified = true;
  }

  // ensure each task point made identical
  for (unsigned i = 0; i < that.TaskSize(); ++i) {
    if (i >= TaskSize()) {
      // that task is larger than this
      Append(*that.task_points[i]);
      modified = true;
    } else if (!task_points[i]->Equals(*that.task_points[i])) {
      // that task point is changed
      Replace(*that.task_points[i], i);
      modified = true;
    }
  }

  // remove if that optional start list is smaller than this one
  while (optional_start_points.size() > that.optional_start_points.size()) {
    RemoveOptionalStart(optional_start_points.size() - 1);
    modified = true;
  }

  // ensure each task point made identical
  for (unsigned i = 0; i < that.optional_start_points.size(); ++i) {
    if (i >= optional_start_points.size()) {
      // that task is larger than this
      AppendOptionalStart(*that.optional_start_points[i]);
      modified = true;
    } else if (!optional_start_points[i]->Equals(*that.optional_start_points[i])) {
      // that task point is changed
      ReplaceOptionalStart(*that.optional_start_points[i], i);
      modified = true;
    }
  }

  mat_points.CloneFrom(that.mat_points, task_behaviour, ordered_behaviour);

  if (modified)
    UpdateGeometry();
    // @todo also re-scan task sample state,
    // potentially resetting task

  return modified;
}

bool
OrderedTask::RelocateOptionalStart(const unsigned position, const Waypoint& waypoint)
{
  if (position >= optional_start_points.size())
    return false;

  OrderedTaskPoint *new_tp =
    optional_start_points[position]->Clone(task_behaviour, ordered_behaviour,
                                           &waypoint);
  delete optional_start_points[position];
  optional_start_points[position]= new_tp;
  return true;
}

bool
OrderedTask::Relocate(const unsigned position, const Waypoint& waypoint)
{
  if (position >= TaskSize())
    return false;

  OrderedTaskPoint *new_tp = task_points[position]->Clone(task_behaviour,
                                                  ordered_behaviour,
                                                  &waypoint);
  bool success = Replace(*new_tp, position);
  delete new_tp;
  return success;
}

void
OrderedTask::SetFactory(const TaskFactoryType the_factory)
{
  // detect no change
  if (factory_mode == the_factory)
    return;

  if (the_factory != TaskFactoryType::MIXED) {
    // can switch from anything to mixed, otherwise need reset
    Reset();

    /// @todo call into task_events to ask if reset is desired on
    /// factory change
  }
  factory_mode = the_factory;

  delete active_factory;
  active_factory = CreateTaskFactory(factory_mode, *this, task_behaviour);
  active_factory->UpdateOrderedTaskBehaviour(ordered_behaviour);
}

void 
OrderedTask::SetOrderedTaskBehaviour(const OrderedTaskBehaviour& ob)
{
  ordered_behaviour = ob;

  for (auto tp : task_points)
    tp->SetOrderedTaskBehaviour(ob);

  for (auto tp : optional_start_points)
    tp->SetOrderedTaskBehaviour(ob);
}

bool 
OrderedTask::IsScored() const
{
  return GetFactoryConstraints().task_scored;
}

std::vector<TaskFactoryType>
OrderedTask::GetFactoryTypes(bool all) const
{
  /// @todo: check transform types if all=false
  std::vector<TaskFactoryType> f_list;
  f_list.push_back(TaskFactoryType::RACING);
  f_list.push_back(TaskFactoryType::AAT);
  f_list.push_back(TaskFactoryType::MAT);
  f_list.push_back(TaskFactoryType::FAI_GENERAL);
  return f_list;
}

void
OrderedTask::RemoveAllPoints()
{
  for (auto i : task_points)
    delete i;

  task_points.clear();

  for (auto i : optional_start_points)
    delete i;

  optional_start_points.clear();
}

void
OrderedTask::Clear()
{
  RemoveAllPoints();

  Reset();
  ordered_behaviour = task_behaviour.ordered_defaults;
  active_factory->UpdateOrderedTaskBehaviour(ordered_behaviour);
}

FlatBoundingBox 
OrderedTask::GetBoundingBox(const GeoBounds &bounds) const
{
  if (!TaskSize()) {
    // undefined!
    return FlatBoundingBox(FlatGeoPoint(0,0),FlatGeoPoint(0,0));
  }

  FlatGeoPoint ll = task_projection.ProjectInteger(bounds.GetSouthWest());
  FlatGeoPoint lr = task_projection.ProjectInteger(bounds.GetSouthEast());
  FlatGeoPoint ul = task_projection.ProjectInteger(bounds.GetNorthWest());
  FlatGeoPoint ur = task_projection.ProjectInteger(bounds.GetNorthEast());
  FlatGeoPoint fmin(std::min(ll.longitude, ul.longitude),
                    std::min(ll.latitude, lr.latitude));
  FlatGeoPoint fmax(std::max(lr.longitude, ur.longitude),
                    std::max(ul.latitude, ur.latitude));
  // note +/- 1 to ensure rounding keeps bb valid 
  fmin.longitude -= 1; fmin.latitude -= 1;
  fmax.longitude += 1; fmax.latitude += 1;
  return FlatBoundingBox (fmin, fmax);
}

void 
OrderedTask::RotateOptionalStarts()
{
  if (!TaskSize())
    return;
  if (!optional_start_points.size()) 
    return;

  SelectOptionalStart(0);
}

void
OrderedTask::SelectOptionalStart(unsigned pos)
{
  assert(pos< optional_start_points.size());

  // put task start onto end
  optional_start_points.push_back(task_points[0]);
  // set task start from top optional item
  task_points[0] = optional_start_points[pos];
  // remove top optional item from list
  optional_start_points.erase(optional_start_points.begin()+pos);

  // update neighbour links
  SetNeighbours(0);
  if (task_points.size()>1)
    SetNeighbours(1);

  // we've changed the task, so update geometry
  UpdateGeometry();
}

void
OrderedTask::UpdateSummary(TaskSummary& ordered_summary) const
{
  ordered_summary.clear();

  ordered_summary.active = active_task_point;

  bool first = true;
  for (const auto *tpp : task_points) {
    const OrderedTaskPoint &tp = *tpp;

    TaskSummaryPoint tsp;
    tsp.d_planned = tp.GetVectorPlanned().distance;
    if (first) {
      first = false;
      tsp.achieved = tp.HasExited();
    } else {
      tsp.achieved = tp.HasSampled();
    }
    ordered_summary.append(tsp);
  }

  if (stats.total.remaining.IsDefined() && stats.total.planned.IsDefined())
    ordered_summary.update(stats.total.remaining.GetDistance(),
                           stats.total.planned.GetDistance());
}

void
OrderedTask::FillMatPoints(const Waypoints &wps, bool update_geometry)
{
  ClearMatPoints();

  if (GetFactoryType() == TaskFactoryType::MAT) {
    mat_points.FillMatPoints(wps, GetFactory());

    if (update_geometry)
      UpdateGeometry();
  }
}
