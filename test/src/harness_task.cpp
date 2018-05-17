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

#include "harness_task.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/Factory/Constraints.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/SymmetricSectorZone.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Util/StaticArray.hxx"
#include "test_debug.hpp"

#include "harness_waypoints.hpp"
#include <string.h>

class SectorZone;
class LineSectorZone;
class KeyholeZone;
class AnnularSectorZone;

class ObservationZoneVisitorPrint
{
public:
  void Visit(const KeyholeZone& oz) {
    printf("# kehole zone\n");
  }
  void Visit(const SectorZone& oz) {
    printf("# sector zone\n");
  }
  void Visit(const AnnularSectorZone& oz) {
    printf("# annular sector zone\n");
  }
  void Visit(const LineSectorZone& oz) {
    printf("# line zone\n");
  }
  void Visit(const CylinderZone& oz) {
    printf("# cylinder zone\n");
  }

  void Visit(const SymmetricSectorZone &oz) {
    printf("# symmetric quadrant\n");
  }

  void Visit(const ObservationZonePoint &oz) {
    switch (oz.GetShape()) {
    case ObservationZone::Shape::FAI_SECTOR:
    case ObservationZone::Shape::SECTOR:
      Visit((const SectorZone &)oz);
      break;

    case ObservationZone::Shape::LINE:
      Visit((const LineSectorZone &)oz);
      break;

    case ObservationZone::Shape::MAT_CYLINDER:
    case ObservationZone::Shape::CYLINDER:
      Visit((const CylinderZone &)oz);
      break;

    case ObservationZone::Shape::CUSTOM_KEYHOLE:
    case ObservationZone::Shape::DAEC_KEYHOLE:
    case ObservationZone::Shape::BGAFIXEDCOURSE:
    case ObservationZone::Shape::BGAENHANCEDOPTION:
    case ObservationZone::Shape::BGA_START:
      Visit((const KeyholeZone &)oz);
      break;

    case ObservationZone::Shape::ANNULAR_SECTOR:
      Visit((const AnnularSectorZone &)oz);
      break;

    case ObservationZone::Shape::SYMMETRIC_QUADRANT:
      Visit((const SymmetricSectorZone &)oz);
      break;
    }
  }
};

class TaskPointVisitorPrint: public TaskPointConstVisitor
{
public:
  virtual void Visit(const TaskPoint& tp) override {
    switch (tp.GetType()) {
    case TaskPointType::UNORDERED:
      printf("# got a tp\n");
      break;

    case TaskPointType::FINISH:
      printf("# got an ftp\n");
      ozv.Visit(((const FinishPoint &)tp).GetObservationZone());
      break;

    case TaskPointType::START:
      printf("# got an stp\n");
      ozv.Visit(((const StartPoint &)tp).GetObservationZone());
      break;

    case TaskPointType::AAT:
      printf("# got an aat\n");
      ozv.Visit(((const AATPoint &)tp).GetObservationZone());
      break;

    case TaskPointType::AST:
      printf("# got an ast\n");
      ozv.Visit(((const ASTPoint &)tp).GetObservationZone());
      break;
    }
  }

private:
  ObservationZoneVisitorPrint ozv;
};

void test_note(const char* text)
{
  if (verbose) {
    printf("%s",text);
  }
}

void
task_report(const TaskManager &task_manager, const char *text)
{
  AircraftState ac;
  if (verbose) {
    printf("%s",text);

    const AbstractTask *task = task_manager.GetActiveTask();
    if (task != NULL) {
      switch (task->GetType()) {
      case TaskType::NONE:
        printf("# task is none\n");
        break;

      case TaskType::ORDERED:
        printf("# task is ordered\n");
        task_manager.GetFactory().UpdateGeometry();
        break;

      case TaskType::ABORT:
        printf("# task is abort\n");
        break;

      case TaskType::GOTO:
        printf("# task is goto\n");
        break;
      }

      TaskPointVisitorPrint tpv;
      task->AcceptTaskPointVisitor(tpv);
      printf("# - dist nominal %g\n",
             (double)task->GetStats().distance_nominal);

      if (task->GetType() == TaskType::ORDERED &&
          task->GetStats().distance_max > task->GetStats().distance_min) {
        printf("# - dist max %g\n", (double)task->GetStats().distance_max);
        printf("# - dist min %g\n", (double)task->GetStats().distance_min);
      }
    }

    PrintHelper::taskmanager_print(task_manager, ac);
  }
  if (interactive>1) {
    WaitPrompt();
  }
}


bool test_task_manip(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  if (!test_task_mixed(task_manager, waypoints)) {
    return false;
  }
  AbstractTaskFactory &fact = task_manager.GetFactory();

  task_report(task_manager, "# removing tp 2\n");
  if (!fact.Remove(2)) {
    return false;
  }

  task_report(task_manager, "# removing tp 0\n");
  if (!fact.Remove(0)) {
    return false;
  }

  task_report(task_manager, "# removing tp -1 (illegal)\n");
  if (fact.Remove(0-1)) {
    return false;
  }

  task_report(task_manager, "# removing tp 50 (illegal)\n");
  if (fact.Remove(50)) {
    return false;
  }

  OrderedTaskPoint *tp;
  WaypointPtr wp;

  task_report(task_manager, "# inserting at 3\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AST_CYLINDER,
                                 std::move(wp));
    if (!fact.Insert(*tp,3)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at 2 (no morph)\n");
  wp = waypoints.LookupId(9);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AST_CYLINDER,
                                 std::move(wp));
    if (!fact.Replace(*tp,2)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at 2 (morph)\n");
  wp = waypoints.LookupId(9);
  if (wp) {
    tp = fact.CreateStart(std::move(wp));
    if (!fact.Replace(*tp,2)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at 0 (morph this)\n");
  wp = waypoints.LookupId(12);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AST_CYLINDER,
                                 std::move(wp));
    if (!fact.Replace(*tp,0)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at end (morph this)\n");
  wp = waypoints.LookupId(14);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AST_CYLINDER,
                                 std::move(wp));
    if (!fact.Replace(*tp,task_manager.TaskSize()-1)) return false;
    delete tp;
  }

  task_report(task_manager, "# removing finish point\n");
  if (!fact.Remove(task_manager.TaskSize()-1)) {
    return false;
  }

  task_report(task_manager, "# inserting at 50 (equivalent to append)\n");
  wp = waypoints.LookupId(8);
  if (wp) {
    tp = fact.CreateFinish(std::move(wp));
    if (!fact.Insert(*tp,50)) return false;
    delete tp;
  }

  task_report(task_manager, "# inserting at 0 (morph this)\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    tp = fact.CreateFinish(std::move(wp));
    if (!fact.Insert(*tp,0)) return false;
    delete tp;
  }

  task_report(task_manager, "# inserting at 2 (morph this)\n");
  wp = waypoints.LookupId(4);
  if (wp) {
    tp = fact.CreateStart(std::move(wp));
    if (!fact.Insert(*tp,2)) return false;
    delete tp;
  }

  task_report(task_manager, "# inserting at 2 (direct)\n");
  wp = waypoints.LookupId(6);
  if (wp) {
    tp = fact.CreateIntermediate(std::move(wp));
    if (!fact.Insert(*tp,2,false)) return false;
    delete tp;
  }

  task_report(task_manager, "# checking task\n");

  fact.UpdateStatsGeometry();

  if (task_manager.CheckOrderedTask()) {
    task_manager.Reset();
    task_manager.SetActiveTaskPoint(0);
    task_manager.Resume();
  } else {
    return false;
  }
  return true;
}

bool test_task_type_manip(TaskManager& task_manager,
                     const Waypoints &waypoints, unsigned n_points)
{
  if (!test_task_random_RT_AAT_FAI(task_manager, waypoints, n_points))
    return false;

  switch (rand() %4) {
  case 0:
    task_manager.SetFactory(TaskFactoryType::AAT);
    test_note("# switched FACTORY TYPE to AAT\n");
    break;
  case 1:
    task_manager.SetFactory(TaskFactoryType::RACING);
    test_note("# switched FACTORY TYPE to RT\n");
    break;
  case 2:
    task_manager.SetFactory(TaskFactoryType::FAI_GENERAL);
    test_note("# switched FACTORY TYPE to FAI GENERAL\n");
    break;
  case 3:
    task_manager.SetFactory(TaskFactoryType::MAT);
    test_note("# switched FACTORY TYPE to MAT\n");
    break;
  default:
    test_note("# unknown task type\n");
  }

  AbstractTaskFactory &fact = task_manager.GetFactory();
  fact.MutateTPsToTaskType();
  fact.UpdateStatsGeometry();

  test_note("# checking mutated start..\n");
  if (!fact.IsValidStartType(fact.GetType(task_manager.GetOrderedTask().GetTaskPoint(0))))
    return false;


  char tmp[255];
  sprintf(tmp, "# checking mutated intermediates.  task_size():%d..\n",
      task_manager.TaskSize());
  test_note(tmp);

  for (unsigned i = 1; i < (task_manager.TaskSize() - 1); i++) {
    sprintf(tmp, "# checking mutated intermediate point %d..\n", i);
    test_note(tmp);
    if (!fact.IsValidIntermediateType(fact.GetType(task_manager.GetOrderedTask().GetTaskPoint(i))))
      return false;
  }

  test_note("# checking mutated finish..\n");
  if (!fact.IsValidFinishType(
      fact.GetType(task_manager.GetOrderedTask().GetTaskPoint(task_manager.TaskSize() - 1))))
    return false;

  test_note("# validating task..\n");
  if (!fact.Validate()) {
    return false;
  }
  test_note("# checking task..\n");
  if (!task_manager.CheckOrderedTask()) {
    return false;
  }

  if (task_manager.GetOrderedTask().GetFactoryType() ==
                                      TaskFactoryType::FAI_GENERAL) {
    test_note("# checking OZs for FAI task..\n");
    if (!fact.ValidateFAIOZs())
      return false;
  }

  if (task_manager.GetOrderedTask().GetFactoryType() ==
                                      TaskFactoryType::MAT) {
    test_note("# checking OZs for MAT task..\n");
    if (!fact.ValidateMATOZs())
      return false;
  }
  return true;
}

bool test_task_mixed(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  OrderedTaskPoint *tp;
  WaypointPtr wp;

  task_manager.SetFactory(TaskFactoryType::MIXED);
  AbstractTaskFactory &fact = task_manager.GetFactory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    tp = fact.CreateStart(TaskPointFactoryType::START_LINE, std::move(wp));
    if (tp->GetObservationZone().GetShape() == ObservationZone::Shape::CYLINDER) {
      CylinderZone &cz = (CylinderZone &)tp->GetObservationZone();
      cz.SetRadius(5000.0);
    }
    if (!fact.Append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.LookupId(2);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AST_CYLINDER,
                                 std::move(wp));
    if (!fact.Append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AAT_CYLINDER,
                                 std::move(wp));
    if (tp->GetObservationZone().GetShape() == ObservationZone::Shape::CYLINDER) {
      CylinderZone &cz = (CylinderZone &)tp->GetObservationZone();
      cz.SetRadius(30000.0);
    }
    if (!fact.Append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(4);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AAT_CYLINDER,
                                 std::move(wp));
    if (!fact.Append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(5);
  if (wp) {
    tp = fact.CreateIntermediate(TaskPointFactoryType::AAT_CYLINDER,
                                 std::move(wp));
    if (tp->GetObservationZone().GetShape() == ObservationZone::Shape::CYLINDER) {
      CylinderZone &cz = (CylinderZone &)tp->GetObservationZone();
      cz.SetRadius(30000.0);
    }
    if (!fact.Append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    tp = fact.CreateFinish(TaskPointFactoryType::FINISH_LINE, std::move(wp));
    if (!fact.Append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;
}


bool test_task_fai(TaskManager& task_manager,
                   const Waypoints &waypoints)
{
  task_manager.SetFactory(TaskFactoryType::FAI_GENERAL);
  AbstractTaskFactory &fact = task_manager.GetFactory();
  WaypointPtr wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateStart(std::move(wp));
    if (!fact.Append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.LookupId(2);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateIntermediate(std::move(wp));
    if (!fact.Append(*tp, false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateIntermediate(std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateFinish(std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;
}


bool test_task_aat(TaskManager& task_manager,
                   const Waypoints &waypoints)
{
  task_manager.SetFactory(TaskFactoryType::AAT);
  AbstractTaskFactory &fact = task_manager.GetFactory();
  WaypointPtr wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateStart(std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(2);
  if (wp) {
    OrderedTaskPoint* tp = fact.CreateIntermediate(TaskPointFactoryType::AAT_CYLINDER,
                                                   std::move(wp));
    if (tp->GetObservationZone().GetShape() == ObservationZone::Shape::CYLINDER) {
      CylinderZone &cz = (CylinderZone &)tp->GetObservationZone();
      cz.SetRadius(30000.0);
    }
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    OrderedTaskPoint* tp = fact.CreateIntermediate(TaskPointFactoryType::AAT_CYLINDER,
                                                   std::move(wp));
    if (tp->GetObservationZone().GetShape() == ObservationZone::Shape::CYLINDER) {
      CylinderZone &cz = (CylinderZone &)tp->GetObservationZone();
      cz.SetRadius(40000.0);
    }
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateFinish(std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task..\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;
}

static bool
test_task_mat(TaskManager &task_manager, const Waypoints &waypoints)
{
  task_manager.SetFactory(TaskFactoryType::MAT);
  AbstractTaskFactory &fact = task_manager.GetFactory();
  WaypointPtr wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateStart(std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(2);
  if (wp) {
    OrderedTaskPoint* tp = fact.CreateIntermediate(TaskPointFactoryType::MAT_CYLINDER,
                                                   std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    OrderedTaskPoint* tp = fact.CreateIntermediate(TaskPointFactoryType::MAT_CYLINDER,
                                                   std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateFinish(std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task..\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;
}


bool test_task_or(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  WaypointPtr wp;

  task_manager.SetFactory(TaskFactoryType::FAI_OR);
  AbstractTaskFactory &fact = task_manager.GetFactory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateStart(std::move(wp));
    if (!fact.Append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.LookupId(2);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateIntermediate(std::move(wp));
    if (!fact.Append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateFinish(std::move(wp));
    if (!fact.Append(*tp)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task..\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;

}


bool test_task_dash(TaskManager& task_manager,
                    const Waypoints &waypoints)
{
  WaypointPtr wp;

  task_manager.SetFactory(TaskFactoryType::TOURING);
  AbstractTaskFactory &fact = task_manager.GetFactory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateStart(std::move(wp));
    if (!fact.Append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(3);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateFinish(std::move(wp));
    if (!fact.Append(*tp)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task..\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;

}


bool test_task_fg(TaskManager& task_manager,
                  const Waypoints &waypoints)
{
  WaypointPtr wp;

  task_manager.SetFactory(TaskFactoryType::FAI_GOAL);
  AbstractTaskFactory &fact = task_manager.GetFactory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.LookupId(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateStart(std::move(wp));
    if (!fact.Append(*tp, false)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.LookupId(6);
  if (wp) {
    OrderedTaskPoint *tp = fact.CreateFinish(std::move(wp));
    if (!fact.Append(*tp, false)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# checking task..\n");
  if (!fact.Validate()) {
    return false;
  }

  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;
}

WaypointPtr
random_waypoint(const Waypoints &waypoints)
{
  static unsigned id_last = 0;
  unsigned id = 0;
  do {
    id = rand() % waypoints.size()+1;
  } while (id==id_last);
  id_last = id;
  return waypoints.LookupId(id);  
}

static TaskPointFactoryType
GetRandomType(const LegalPointSet &l)
{
  StaticArray<TaskPointFactoryType, LegalPointSet::N> types;
  l.CopyTo(std::back_inserter(types));
  return types[(rand() % types.size())];
}

bool test_task_random(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned num_points)
{
  WaypointPtr wp;

  OrderedTaskPoint *tp;

  task_manager.SetFactory(TaskFactoryType::MIXED);
  AbstractTaskFactory &fact = task_manager.GetFactory();

  task_report(task_manager, "# adding start\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    const TaskPointFactoryType s = GetRandomType(fact.GetStartTypes());

    tp = fact.CreateStart(s, wp);
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_manager.SetActiveTaskPoint(0);
  task_manager.Resume();

  for (unsigned i=0; i<num_points; i++) {
    task_report(task_manager, "# adding intermediate\n");
    wp = random_waypoint(waypoints);
    if (wp) {
      const TaskPointFactoryType s = GetRandomType(fact.GetIntermediateTypes());

      tp = fact.CreateIntermediate(s,std::move(wp));
      if (!fact.Append(*tp,false)) {
        return false;
      }
      delete tp;
    }
  }

  task_report(task_manager, "# adding finish\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    const TaskPointFactoryType s = GetRandomType(fact.GetFinishTypes());

    tp = fact.CreateFinish(s,std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  task_report(task_manager, "# validating task..\n");
  if (!fact.Validate()) {
    return false;
  }
  task_report(task_manager, "# checking task..\n");
  if (!task_manager.CheckOrderedTask()) {
    return false;
  }
  return true;
}

bool test_task_random_RT_AAT_FAI(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned _num_points)
{
  WaypointPtr wp;

  OrderedTaskPoint *tp;
  char tmp[255];
  char tskType[20];
  tskType[0] = '\0';


  switch (rand() %3) {
  case 0:
    task_manager.SetFactory(TaskFactoryType::AAT);
    strcpy(tskType,"AAT");
    test_note("# creating random AAT task\n");
    break;
  case 1:
    task_manager.SetFactory(TaskFactoryType::RACING);
    strcpy(tskType,"RT");
    test_note("# creating random RT task\n");
    break;
  case 2:
    task_manager.SetFactory(TaskFactoryType::FAI_GENERAL);
    strcpy(tskType,"FAI");
    test_note("# creating random FAI GENERAL\n");
    break;
  }

  AbstractTaskFactory &fact = task_manager.GetFactory();

  //max points includes start & finish
  const TaskFactoryConstraints &constraints =
    task_manager.GetOrderedTask().GetFactoryConstraints();
  const unsigned num_points_total =
    std::max(constraints.min_points,
             _num_points % constraints.max_points) + 1;
  const unsigned num_int_points = num_points_total - 2;

  test_note("# adding start\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    const TaskPointFactoryType s = GetRandomType(fact.GetStartTypes());

    tp = fact.CreateStart(s,std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  for (unsigned i=0; i<num_int_points; i++) {
    test_note("# adding intermediate\n");
    wp = random_waypoint(waypoints);
    if (wp) {
      const TaskPointFactoryType s = GetRandomType(fact.GetIntermediateTypes());

      tp = fact.CreateIntermediate(s,std::move(wp));
      if (!fact.Append(*tp,false)) {
        return false;
      }
      delete tp;
    }
  }

  test_note("# adding finish\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    const TaskPointFactoryType s = GetRandomType(fact.GetFinishTypes());

    tp = fact.CreateFinish(s,std::move(wp));
    if (!fact.Append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  fact.UpdateStatsGeometry();

  test_note("# validating task..\n");
  if (!fact.Validate()) {
    return false;
  }
  if (task_manager.GetOrderedTask().GetFactoryType()
      == TaskFactoryType::FAI_GENERAL)
  {
    test_note("# checking OZs for FAI General..\n");
    if (!fact.ValidateFAIOZs())
      return false;
  }

  if (task_manager.GetOrderedTask().GetFactoryType()
      == TaskFactoryType::MAT)
  {
    test_note("# checking OZs for MAT General..\n");
    if (!fact.ValidateMATOZs())
      return false;
  }
  task_manager.Resume();
  sprintf(tmp, "# SUCCESS CREATING %s task! task_size():%d..\n",
      tskType,
      task_manager.TaskSize());
  test_note(tmp);
  return true;
}

bool test_task(TaskManager& task_manager,
               const Waypoints &waypoints,
               int test_num)
{
  unsigned n_points = rand()%8+1;
  switch (test_num) {
  case 0:
    return test_task_mixed(task_manager,waypoints);
  case 1:
    return test_task_fai(task_manager,waypoints);
  case 2:
    return test_task_aat(task_manager,waypoints);
  case 3:
    return test_task_or(task_manager,waypoints);
  case 4:
    return test_task_dash(task_manager,waypoints);
  case 5:
    return test_task_fg(task_manager,waypoints);
  case 6:
    return test_task_manip(task_manager,waypoints);
  case 7:
    return test_task_random(task_manager,waypoints,n_points);
  case 8:
    return test_task_mat(task_manager,waypoints);
  default:
    return false;
  }
}


const char* task_name(int test_num)
{
  switch (test_num) {
  case 0:
    return "mixed";
  case 1:
    return "fai";
  case 2:
    return "aat";
  case 3:
    return "or";
  case 4:
    return "dash";
  case 5:
    return "fg";
  case 6:
    return "manip";
  case 7:
    return "random";
  default:
    return "unknown";
  }
}


