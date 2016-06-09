/* Copyright_License {

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

#include "OrderedTask.hpp"
#include "Task/TaskEvents.hpp"
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
#include "Task/Factory/Constraints.hpp"

#include "Waypoint/Waypoints.hpp"
#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "Task/Stats/TaskSummary.hpp"
#include "Task/PathSolvers/TaskDijkstraMin.hpp"
#include "Task/PathSolvers/TaskDijkstraMax.hpp"
#include "Task/ObservationZones/ObservationZoneClient.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"

/**
 * According to "FAI Sporting Code / Annex A to Section 3 - Gliding",
 * 6.3.1c and 6.3.2dii, the radius of the "start/finish ring" must be
 * subtracted from the task distance.  This flag controls whether this
 * behaviour is enabled.
 *
 * Currently, it is always enabled, but at some point, we may want to
 * make it optional.
 */
constexpr bool subtract_start_finish_cylinder_radius = true;

/**
 * Determine the cylinder radius if this is a CylinderZone.  If not,
 * return -1.
 */
gcc_pure
static double
GetCylinderRadiusOrMinusOne(const ObservationZone &oz)
{
  return oz.GetShape() == ObservationZone::Shape::CYLINDER
    ? ((const CylinderZone &)oz).GetRadius()
    : -1;
}

/**
 * Determine the cylinder radius if this is a CylinderZone.  If not,
 * return -1.
 */
gcc_pure
static double
GetCylinderRadiusOrMinusOne(const ObservationZoneClient &p)
{
  return GetCylinderRadiusOrMinusOne(p.GetObservationZone());
}

OrderedTask::OrderedTask(const TaskBehaviour &tb)
  :AbstractTask(TaskType::ORDERED, tb),
   taskpoint_start(nullptr),
   taskpoint_finish(nullptr),
   factory_mode(tb.task_type_default),
   active_factory(nullptr),
   ordered_settings(tb.ordered_defaults),
   dijkstra_min(nullptr), dijkstra_max(nullptr)
{
  ClearName();
  active_factory = CreateTaskFactory(factory_mode, *this, task_behaviour);
  active_factory->UpdateOrderedTaskSettings(ordered_settings);
}

OrderedTask::~OrderedTask()
{
  RemoveAllPoints();

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
                       const FlatProjection &projection)
{
  for (auto i : points)
    i->UpdateOZ(projection);
}

void
OrderedTask::UpdateStatsGeometry()
{
  ScanStartFinish();

  stats.task_valid = CheckTask();
  stats.has_targets = stats.task_valid && HasTargets();
  stats.is_mat = GetFactoryType() == TaskFactoryType::MAT;
  stats.has_optional_starts = stats.task_valid && HasOptionalStarts();
}

void
OrderedTask::UpdateGeometry()
{
  UpdateStatsGeometry();

  if (task_points.empty())
    return;

  auto &first = *task_points.front();

  first.ScanActive(*task_points[active_task_point]);

  // scan location of task points
  GeoBounds bounds(first.GetLocation());
  for (const auto *tp : task_points)
    tp->ScanBounds(bounds);

  // ... and optional start points
  for (const OrderedTaskPoint *tp : optional_start_points)
    tp->ScanBounds(bounds);

  // projection can now be determined
  task_projection = TaskProjection(bounds);

  // update OZ's for items that depend on next-point geometry
  UpdateObservationZones(task_points, task_projection);
  UpdateObservationZones(optional_start_points, task_projection);

  // now that the task projection is stable, and oz is stable,
  // calculate the bounding box in projected coordinates
  for (const auto tp : task_points)
    tp->UpdateBoundingBox(task_projection);

  for (const auto tp : optional_start_points)
    tp->UpdateBoundingBox(task_projection);

  // update stats so data can be used during task construction
  /// @todo this should only be done if not flying! (currently done with has_entered)
  if (!task_points.front()->HasEntered()) {
    UpdateStatsDistances(GeoPoint::Invalid(), true);
    if (HasFinish()) {
      /// @todo: call AbstractTask::update stats methods with fake state
      /// so stats are updated
    }
  }

  force_full_update = true;
}

// TIMES

double
OrderedTask::ScanTotalStartTime()
{
  if (task_points.empty())
    return -1;

  return task_points.front()->GetEnteredState().time;
}

double
OrderedTask::ScanLegStartTime()
{
  if (active_task_point > 0)
    return task_points[active_task_point-1]->GetEnteredState().time;

  return -1;
}

// DISTANCES

inline bool
OrderedTask::RunDijsktraMin(const GeoPoint &location)
{
  const unsigned task_size = TaskSize();
  if (task_size < 2)
    return false;

  if (dijkstra_min == nullptr)
    dijkstra_min = new TaskDijkstraMin();
  TaskDijkstraMin &dijkstra = *dijkstra_min;

  const unsigned active_index = GetActiveIndex();
  dijkstra.SetTaskSize(task_size - active_index);
  for (unsigned i = active_index; i != task_size; ++i) {
    const SearchPointVector &boundary = task_points[i]->GetSearchPoints();
    dijkstra.SetBoundary(i - active_index, boundary);
  }

  SearchPoint ac(location, task_projection);
  if (!dijkstra.DistanceMin(ac))
    return false;

  for (unsigned i = active_index; i != task_size; ++i)
    SetPointSearchMin(i, dijkstra.GetSolution(i - active_index));

  return true;
}

inline double
OrderedTask::ScanDistanceMin(const GeoPoint &location, bool full)
{
  if (!full && location.IsValid() && last_min_location.IsValid() &&
      DistanceIsSignificant(location, last_min_location)) {
    const TaskWaypoint *active = GetActiveTaskPoint();
    if (active != nullptr) {
      const GeoPoint &target = active->GetWaypoint().location;
      const unsigned last_distance =
        (unsigned)last_min_location.Distance(target);
      const unsigned cur_distance =
        (unsigned)location.Distance(target);

      /* do the full scan only if the distance to the active task
         point has changed by more than 5%, otherwise we don't expect
         any relevant changes */
      if (last_distance < 2000 || cur_distance < 2000 ||
          last_distance * 20 >= cur_distance * 21 ||
          cur_distance * 20 >= last_distance * 21)
        full = true;
    }
  }

  if (full) {
    RunDijsktraMin(location);
    last_min_location = location;
  }

  return task_points.front()->ScanDistanceMin();
}

inline bool
OrderedTask::RunDijsktraMax()
{
  const unsigned task_size = TaskSize();
  if (task_size < 2)
    return false;

  if (dijkstra_max == nullptr)
    dijkstra_max = new TaskDijkstraMax();
  TaskDijkstraMax &dijkstra = *dijkstra_max;

  const unsigned active_index = GetActiveIndex();
  dijkstra.SetTaskSize(task_size);
  for (unsigned i = 0; i != task_size; ++i) {
    const SearchPointVector &boundary = i == active_index
      /* since one can still travel further in the current sector, use
         the full boundary here */
      ? task_points[i]->GetBoundaryPoints()
      : task_points[i]->GetSearchPoints();
    dijkstra_max->SetBoundary(i, boundary);
  }

  double start_radius(-1), finish_radius(-1);
  if (subtract_start_finish_cylinder_radius) {
    /* to subtract the start/finish cylinder radius, we use only the
       nominal points (i.e. the cylinder's center), and later replace
       it with a point on the cylinder boundary */

    const auto &start = *task_points.front();
    start_radius = GetCylinderRadiusOrMinusOne(start);
    if (start_radius > 0)
      dijkstra.SetBoundary(0, start.GetNominalPoints());

    const auto &finish = *task_points.back();
    finish_radius = GetCylinderRadiusOrMinusOne(finish);
    if (finish_radius > 0)
      dijkstra.SetBoundary(task_size - 1, finish.GetNominalPoints());
  }

  if (!dijkstra_max->DistanceMax())
    return false;

  for (unsigned i = 0; i != task_size; ++i) {
    SearchPoint solution = dijkstra.GetSolution(i);

    if (i == 0 && start_radius > 0) {
      /* subtract start cylinder radius by finding the intersection
         with the cylinder boundary */
      const GeoPoint &current = task_points.front()->GetLocation();
      const GeoPoint &neighbour = dijkstra.GetSolution(i + 1).GetLocation();
      GeoPoint gp = current.IntermediatePoint(neighbour, start_radius);
      solution = SearchPoint(gp, task_projection);
    }

    if (i == task_size - 1 && finish_radius > 0) {
      /* subtract finish cylinder radius by finding the intersection
         with the cylinder boundary */
      const GeoPoint &current = task_points.back()->GetLocation();
      const GeoPoint &neighbour = dijkstra.GetSolution(i - 1).GetLocation();
      GeoPoint gp = current.IntermediatePoint(neighbour, finish_radius);
      solution = SearchPoint(gp, task_projection);
    }

    SetPointSearchMax(i, solution);
    if (i <= active_index)
      set_tp_search_achieved(i, solution);
  }

  return true;
}

inline double
OrderedTask::ScanDistanceMax()
{
  if (task_points.empty()) // nothing to do!
    return 0;

  assert(active_task_point < task_points.size());

  RunDijsktraMax();

  return task_points.front()->ScanDistanceMax();
}

void
OrderedTask::ScanDistanceMinMax(const GeoPoint &location, bool force,
                                double *dmin, double *dmax)
{
  if (force)
    *dmax = ScanDistanceMax();

  *dmin = ScanDistanceMin(location, force);
}

double
OrderedTask::ScanDistanceNominal()
{
  if (task_points.empty())
    return 0;

  const auto &start = *task_points.front();
  auto d = start.ScanDistanceNominal();

  auto radius = GetCylinderRadiusOrMinusOne(start);
  if (radius > 0 && radius < d)
    d -= radius;

  const auto &finish = *task_points.back();
  radius = GetCylinderRadiusOrMinusOne(finish);
  if (radius > 0 && radius < d)
    d -= radius;

  return d;
}

double
OrderedTask::ScanDistanceScored(const GeoPoint &location)
{
  return task_points.empty()
    ? 0
    : task_points.front()->ScanDistanceScored(location);
}

double
OrderedTask::ScanDistanceRemaining(const GeoPoint &location)
{
  return task_points.empty()
    ? 0
    : task_points.front()->ScanDistanceRemaining(location);
}

double
OrderedTask::ScanDistanceTravelled(const GeoPoint &location)
{
  return task_points.empty()
    ? 0
    : task_points.front()->ScanDistanceTravelled(location);
}

double
OrderedTask::ScanDistancePlanned()
{
  return task_points.empty()
    ? 0
    : task_points.front()->ScanDistancePlanned();
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

  bool last_started = stats.start.task_started;
  const bool last_finished = stats.task_finished;

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

          if (task_events != nullptr)
            task_events->ActiveAdvanced(*task_points[i], i);

          // on sector exit, must update samples since start sector
          // exit transition clears samples
          full_update = true;
        }
      } else if (!last_request_armed && task_advance.NeedToArm()) {
        if (task_events != nullptr)
          task_events->RequestArm(*task_points[i]);
      }
    }
  }

  stats.need_to_arm = task_advance.NeedToArm();

  taskpoint_start->ScanActive(*task_points[active_task_point]);

  stats.task_finished = taskpoint_finish != nullptr &&
    taskpoint_finish->HasEntered();
  stats.start.task_started = TaskStarted();

  if (stats.start.task_started) {
    const AircraftState start_state = taskpoint_start->GetEnteredState();
    stats.start.SetStarted(start_state);

    if (taskpoint_finish != nullptr)
      taskpoint_finish->SetFaiFinishHeight(start_state.altitude - 1000);
  }

  if (task_events != nullptr) {
    if (stats.start.task_started && !last_started)
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

    if (task_events != nullptr)
      task_events->EnterTransition(point);
  }

  if (nearby && point.TransitionExit(state, state_last, task_projection)) {
    transition_exit = true;

    if (task_events != nullptr)
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
      GetOrderedTaskSettings().aat_min_time > 0) {

    CalcMinTarget(state, glide_polar,
                  GetOrderedTaskSettings().aat_min_time + task_behaviour.optimise_targets_margin);

    if (task_behaviour.optimise_targets_bearing &&
        task_points[active_task_point]->GetType() == TaskPointType::AAT) {
      AATPoint *ap = (AATPoint *)task_points[active_task_point];
      // very nasty hack
      TaskOptTarget tot(task_points, active_task_point, state,
                        task_behaviour.glide, glide_polar,
                        *ap, task_projection, taskpoint_start);
      tot.search(0.5);
    }
    retval = true;
  }

  return retval;
}

bool
OrderedTask::UpdateSample(const AircraftState &state,
                          gcc_unused const GlidePolar &glide_polar,
                          gcc_unused const bool full_update)
{
  assert(state.location.IsValid());

  stats.inside_oz = active_task_point < task_points.size() &&
    task_points[active_task_point]->IsInSector(state);

  return true;
}

// TASK

void
OrderedTask::SetNeighbours(unsigned position)
{
  OrderedTaskPoint* prev = nullptr;
  OrderedTaskPoint* next = nullptr;

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
   return nullptr;
 }

 if (task_points[TPindex]->GetType() == TaskPointType::AAT)
   return (AATPoint *)task_points[TPindex];
 else
   return (AATPoint *)nullptr;
}

inline bool
OrderedTask::ScanStartFinish()
{
  /// @todo also check there are not more than one start/finish point
  if (task_points.empty()) {
    taskpoint_start = nullptr;
    taskpoint_finish = nullptr;
    return false;
  }

  taskpoint_start = task_points.front()->GetType() == TaskPointType::START
    ? (StartPoint *)task_points.front()
    : nullptr;

  taskpoint_finish = task_points.size() > 1 &&
    task_points.back()->GetType() == TaskPointType::FINISH
    ? (FinishPoint *)task_points.back()
    : nullptr;

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

  if (position < task_points.size())
    SetNeighbours(position);

  if (position)
    SetNeighbours(position - 1);

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

  return true;
}

bool
OrderedTask::Append(const OrderedTaskPoint &new_tp)
{
  if (!task_points.empty() &&
      (/* is the new_tp allowed in this context? */
       !new_tp.IsPredecessorAllowed() ||
       /* can a tp be appended after the last one? */
       !task_points.back()->IsSuccessorAllowed()))
    return false;

  const unsigned i = task_points.size();
  task_points.push_back(new_tp.Clone(task_behaviour, ordered_settings));
  if (i > 0)
    SetNeighbours(i - 1);
  else {
    // give it a value when we have one tp so it is not uninitialised
    last_min_location = new_tp.GetLocation();
  }

  SetNeighbours(i);
  return true;
}

bool
OrderedTask::AppendOptionalStart(const OrderedTaskPoint &new_tp)
{
  optional_start_points.push_back(new_tp.Clone(task_behaviour,
                                               ordered_settings));
  if (task_points.size() > 1)
    SetNeighbours(0);
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
                     new_tp.Clone(task_behaviour, ordered_settings));

  if (position)
    SetNeighbours(position - 1);

  SetNeighbours(position);
  SetNeighbours(position + 1);

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
  task_points[position] = new_tp.Clone(task_behaviour, ordered_settings);

  if (position)
    SetNeighbours(position - 1);

  SetNeighbours(position);
  if (position + 1 < task_points.size())
    SetNeighbours(position + 1);

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
                                                 ordered_settings);

  SetNeighbours(0);
  return true;
}


void
OrderedTask::SetActiveTaskPoint(unsigned index)
{
  if (index >= task_points.size() || index == active_task_point)
    return;

  task_advance.SetArmed(false);
  active_task_point = index;
  force_full_update = true;
}

TaskWaypoint*
OrderedTask::GetActiveTaskPoint() const
{
  if (active_task_point < task_points.size())
    return task_points[active_task_point];

  return nullptr;
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
  if (!aircraft.location.IsValid() || task_points.empty()) {
    total.Reset();
    leg.Reset();
    return;
  }

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
  if (!aircraft.location.IsValid() || task_points.empty()) {
    total.Reset();
    leg.Reset();
    return;
  }

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
  if (task_points.empty()) {
    total.Reset();
    leg.Reset();
    total_remaining_effective.Reset();
    leg_remaining_effective.Reset();
    return;
  }

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

double
OrderedTask::CalcRequiredGlide(const AircraftState &aircraft,
                               const GlidePolar &glide_polar) const
{
  TaskGlideRequired bgr(task_points, active_task_point, aircraft,
                        task_behaviour.glide, glide_polar);
  return bgr.search(0);
}

bool
OrderedTask::CalcBestMC(const AircraftState &aircraft,
                        const GlidePolar &glide_polar,
                        double &best) const
{
  // note setting of lower limit on mc
  TaskBestMc bmc(task_points, active_task_point, aircraft,
                 task_behaviour.glide, glide_polar);
  return bmc.search(glide_polar.GetMC(), best);
}


bool
OrderedTask::AllowIncrementalBoundaryStats(const AircraftState &aircraft) const
{
  if (active_task_point == 0)
    /* disabled for the start point */
    return false;

  if (task_points[active_task_point]->IsBoundaryScored())
    return true;

  bool in_sector = task_points[active_task_point]->IsInSector(aircraft) ||
    task_points[active_task_point-1]->IsInSector(aircraft);

  return !in_sector;
}

bool
OrderedTask::CalcCruiseEfficiency(const AircraftState &aircraft,
                                  const GlidePolar &glide_polar,
                                  double &val) const
{
  if (AllowIncrementalBoundaryStats(aircraft)) {
    TaskCruiseEfficiency bce(task_points, active_task_point, aircraft,
                             task_behaviour.glide, glide_polar);
    val = bce.search(1);
    return true;
  } else {
    val = 1;
    return false;
  }
}

bool
OrderedTask::CalcEffectiveMC(const AircraftState &aircraft,
                             const GlidePolar &glide_polar,
                             double &val) const
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


inline double
OrderedTask::CalcMinTarget(const AircraftState &aircraft,
                           const GlidePolar &glide_polar,
                           const double t_target)
{
  if (stats.has_targets) {
    // only perform scan if modification is possible
    const auto t_rem = fdim(t_target, stats.total.time_elapsed);

    TaskMinTarget bmt(task_points, active_task_point, aircraft,
                      task_behaviour.glide, glide_polar,
                      t_rem, taskpoint_start);
    auto p = bmt.search(0);
    return p;
  }

  return 0;
}

double
OrderedTask::CalcGradient(const AircraftState &state) const
{
  if (task_points.empty())
    return 0;

  // Iterate through remaining turnpoints
  double distance = 0;
  for (const OrderedTaskPoint *tp : task_points)
    // Sum up the leg distances
    distance += tp->GetVectorRemaining(state.location).distance;

  if (distance <= 0)
    return 0;

  // Calculate gradient to the last turnpoint of the remaining task
  return (state.altitude - task_points.back()->GetElevation()) / distance;
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
  stats.start.task_started = false;
  task_advance.Reset();
  SetActiveTaskPoint(0);
  UpdateStatsGeometry();
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

bool
OrderedTask::HasTargets() const
{
  for (const OrderedTaskPoint *tp : task_points)
    if (tp->HasTarget())
      return true;

  return false;
}

GeoPoint
OrderedTask::GetTaskCenter() const
{
  return task_points.empty()
    ? GeoPoint::Invalid()
    : task_projection.GetCenter();
}

double
OrderedTask::GetTaskRadius() const
{
  return task_points.empty()
    ? 0
    : task_projection.ApproxRadius();
}

OrderedTask*
OrderedTask::Clone(const TaskBehaviour &tb) const
{
  OrderedTask* new_task = new OrderedTask(tb);

  new_task->SetFactory(factory_mode);

  new_task->ordered_settings = ordered_settings;

  for (const OrderedTaskPoint *tp : task_points)
    new_task->Append(*tp);

  for (const OrderedTaskPoint *tp : optional_start_points)
    new_task->AppendOptionalStart(*tp);

  new_task->active_task_point = active_task_point;
  new_task->UpdateGeometry();

  new_task->SetName(GetName());

  return new_task;
}

void
OrderedTask::CheckDuplicateWaypoints(Waypoints& waypoints,
                                     OrderedTaskPointVector& points,
                                     const bool is_task)
{
  for (auto begin = points.cbegin(), end = points.cend(), i = begin;
       i != end; ++i) {
    auto wp = waypoints.CheckExistsOrAppend((*i)->GetWaypointPtr());

    const OrderedTaskPoint *new_tp =
      (*i)->Clone(task_behaviour, ordered_settings, std::move(wp));
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

  SetName(that.GetName());

  // change mode to that one
  SetFactory(that.factory_mode);

  // copy across behaviour
  SetOrderedTaskSettings(that.ordered_settings);

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

  if (modified)
    UpdateGeometry();
    // @todo also re-scan task sample state,
    // potentially resetting task

  return modified;
}

bool
OrderedTask::RelocateOptionalStart(const unsigned position,
                                   WaypointPtr &&waypoint)
{
  if (position >= optional_start_points.size())
    return false;

  OrderedTaskPoint *new_tp =
    optional_start_points[position]->Clone(task_behaviour, ordered_settings,
                                           std::move(waypoint));
  delete optional_start_points[position];
  optional_start_points[position]= new_tp;
  return true;
}

bool
OrderedTask::Relocate(const unsigned position, WaypointPtr &&waypoint)
{
  if (position >= TaskSize())
    return false;

  OrderedTaskPoint *new_tp = task_points[position]->Clone(task_behaviour,
                                                          ordered_settings,
                                                          std::move(waypoint));
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
  active_factory->UpdateOrderedTaskSettings(ordered_settings);

  PropagateOrderedTaskSettings();
}

void
OrderedTask::SetOrderedTaskSettings(const OrderedTaskSettings& ob)
{
  ordered_settings = ob;

  PropagateOrderedTaskSettings();
}

void
OrderedTask::PropagateOrderedTaskSettings()
{
  for (auto tp : task_points)
    tp->SetOrderedTaskSettings(ordered_settings);

  for (auto tp : optional_start_points)
    tp->SetOrderedTaskSettings(ordered_settings);
}

bool
OrderedTask::IsScored() const
{
  return GetFactoryConstraints().task_scored;
}

std::vector<TaskFactoryType>
OrderedTask::GetFactoryTypes(gcc_unused bool all) const
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

  active_task_point = 0;
  taskpoint_start = nullptr;
  taskpoint_finish = nullptr;
  force_full_update = true;
}

void
OrderedTask::Clear()
{
  RemoveAllPoints();

  ClearName();

  Reset();
  ordered_settings = task_behaviour.ordered_defaults;
  active_factory->UpdateOrderedTaskSettings(ordered_settings);
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
  optional_start_points.push_back(task_points.front());
  // set task start from top optional item
  task_points.front() = optional_start_points[pos];
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
