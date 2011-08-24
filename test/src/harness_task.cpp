/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "test_debug.hpp"

#include "Dialogs/dlgTaskHelpers.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"

#include "harness_waypoints.hpp"
#include <string.h>

class ObservationZoneVisitorPrint: public ObservationZoneConstVisitor
{
public:
  virtual void Visit(const KeyholeZone& oz) {
    printf("# kehole zone\n");
  }
  virtual void Visit(const FAISectorZone& oz) {
    printf("# fai sect zone\n");
  }
  virtual void Visit(const SectorZone& oz) {
    printf("# sector zone\n");
  }
  virtual void Visit(const AnnularSectorZone& oz) {
    printf("# annular sector zone\n");
  }
  virtual void Visit(const LineSectorZone& oz) {
    printf("# line zone\n");
  }
  virtual void Visit(const CylinderZone& oz) {
    printf("# cylinder zone\n");
  }
  virtual void Visit(const BGAFixedCourseZone &oz) {
    printf("# bga fixed course zone\n");
  }
  virtual void Visit(const BGAEnhancedOptionZone &oz) {
    printf("# bga enhanded option zone\n");
  }
  virtual void Visit(const BGAStartSectorZone &oz) {
    printf("# bga start sector zone\n");
  }

  void Visit(const ObservationZonePoint &oz) {
    ObservationZoneConstVisitor::Visit(oz);
  }
};

class TaskPointVisitorPrint: public TaskPointConstVisitor
{
public:
  virtual void Visit(const UnorderedTaskPoint& tp) {
    printf("# got a tp\n");
  }
  virtual void Visit(const OrderedTaskPoint& tp) {
    printf("# got an otp\n");
    ozv.Visit(*tp.get_oz());
  }
  virtual void Visit(const FinishPoint& tp) {
    printf("# got an ftp\n");
    ozv.Visit(*tp.get_oz());
  }
  virtual void Visit(const StartPoint& tp) {
    printf("# got an stp\n");
    ozv.Visit(*tp.get_oz());
  }
  virtual void Visit(const AATPoint& tp) {
    printf("# got an aat\n");
    ozv.Visit(*tp.get_oz());
  }
  virtual void Visit(const ASTPoint& tp) {
    printf("# got an ast\n");
    ozv.Visit(*tp.get_oz());
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

void task_report(TaskManager& task_manager, const char* text)
{
  AircraftState ac;
  if (verbose) {
    printf("%s",text);

    const AbstractTask *task = task_manager.get_active_task();
    if (task != NULL) {
      switch (task->type) {
      case TaskInterface::ORDERED:
        printf("# task is ordered\n");
        break;

      case TaskInterface::ABORT:
        printf("# task is abort\n");
        break;

      case TaskInterface::GOTO:
        printf("# task is goto\n");
        break;
      }

      TaskPointVisitorPrint tpv;
      task->tp_CAccept(tpv);
      printf("# - dist nominal %g\n",
             (double)task->GetStats().distance_nominal);

      if (task->type == TaskInterface::ORDERED &&
          task->GetStats().distance_max > task->GetStats().distance_min) {
        printf("# - dist max %g\n", (double)task->GetStats().distance_max);
        printf("# - dist min %g\n", (double)task->GetStats().distance_min);
      }
    }

    PrintHelper::taskmanager_print(task_manager, ac);
  }
  if (interactive>1) {
    wait_prompt();
  }
}


bool test_task_manip(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  if (!test_task_mixed(task_manager, waypoints)) {
    return false;
  }
  AbstractTaskFactory &fact = task_manager.get_factory();

  task_report(task_manager, "# removing tp 2\n");
  if (!fact.remove(2)) {
    return false;
  }

  task_report(task_manager, "# removing tp 0\n");
  if (!fact.remove(0)) {
    return false;
  }

  task_report(task_manager, "# removing tp -1 (illegal)\n");
  if (fact.remove(0-1)) {
    return false;
  }

  task_report(task_manager, "# removing tp 50 (illegal)\n");
  if (fact.remove(50)) {
    return false;
  }

  OrderedTaskPoint *tp;
  const Waypoint *wp;

  task_report(task_manager, "# inserting at 3\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact.insert(*tp,3)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at 2 (no morph)\n");
  wp = waypoints.lookup_id(9);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact.replace(*tp,2)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at 2 (morph)\n");
  wp = waypoints.lookup_id(9);
  if (wp) {
    tp = fact.createStart(*wp);
    if (!fact.replace(*tp,2)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at 0 (morph this)\n");
  wp = waypoints.lookup_id(12);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact.replace(*tp,0)) return false;
    delete tp;
  }

  task_report(task_manager, "# auto-replacing at end (morph this)\n");
  wp = waypoints.lookup_id(14);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact.replace(*tp,task_manager.task_size()-1)) return false;
    delete tp;
  }

  task_report(task_manager, "# removing finish point\n");
  if (!fact.remove(task_manager.task_size()-1)) {
    return false;
  }

  task_report(task_manager, "# inserting at 50 (equivalent to append)\n");
  wp = waypoints.lookup_id(8);
  if (wp) {
    tp = fact.createFinish(*wp);
    if (!fact.insert(*tp,50)) return false;
    delete tp;
  }

  task_report(task_manager, "# inserting at 0 (morph this)\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact.createFinish(*wp);
    if (!fact.insert(*tp,0)) return false;
    delete tp;
  }

  task_report(task_manager, "# inserting at 2 (morph this)\n");
  wp = waypoints.lookup_id(4);
  if (wp) {
    tp = fact.createStart(*wp);
    if (!fact.insert(*tp,2)) return false;
    delete tp;
  }

  task_report(task_manager, "# inserting at 2 (direct)\n");
  wp = waypoints.lookup_id(6);
  if (wp) {
    tp = fact.createIntermediate(*wp);
    if (!fact.insert(*tp,2,false)) return false;
    delete tp;
  }

  task_report(task_manager, "# checking task\n");

  if (task_manager.check_ordered_task()) {
    task_manager.reset();
    task_manager.setActiveTaskPoint(0);
    task_manager.resume();
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

  AbstractTaskFactory &fact = task_manager.get_factory();

  switch (rand() %3) {
  case 0:
    task_manager.set_factory(TaskBehaviour::FACTORY_AAT);
    test_note("# switched FACTORY TYPE to AAT\n");
    break;
  case 1:
    task_manager.set_factory(TaskBehaviour::FACTORY_RT);
    test_note("# switched FACTORY TYPE to RT\n");
    break;
  case 2:
    task_manager.set_factory(TaskBehaviour::FACTORY_FAI_GENERAL);
    test_note("# switched FACTORY TYPE to FAI GENERAL\n");
    break;
  default:
    test_note("# unknown task type\n");
  }

  fact.mutate_tps_to_task_type();

  test_note("# checking mutated start..\n");
  if (!fact.validStartType(
      fact.getType(*task_manager.get_ordered_task().getTaskPoint(0))))
    return false;


  char tmp[255];
  sprintf(tmp, "# checking mutated intermediates.  task_size():%d..\n",
      task_manager.task_size());
  test_note(tmp);

  for (unsigned i = 1; i < (task_manager.task_size() - 1); i++) {
    sprintf(tmp, "# checking mutated intermediate point %d..\n", i);
    test_note(tmp);
    if (!fact.validIntermediateType(
        fact.getType(*task_manager.get_ordered_task().getTaskPoint(i))))
      return false;
  }

  test_note("# checking mutated finish..\n");
  if (!fact.validFinishType(
      fact.getType(*task_manager.get_ordered_task().getTaskPoint(
          task_manager.task_size() - 1))))
    return false;

  test_note("# validating task..\n");
  if (!fact.validate()) {
    return false;
  }
  test_note("# checking task..\n");
  if (!task_manager.check_ordered_task()) {
    return false;
  }

  if (task_manager.get_ordered_task().get_factory_type() ==
                                      TaskBehaviour::FACTORY_FAI_GENERAL) {
    test_note("# checking OZs for FAI task..\n");
    if (!fact.validateFAIOZs())
      return false;
  }

  return true;
}

bool test_task_mixed(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  const TaskProjection &projection =
    task_manager.get_ordered_task().get_task_projection();

  OrderedTaskPoint *tp;
  const Waypoint *wp;

  task_manager.set_factory(TaskBehaviour::FACTORY_MIXED);
  AbstractTaskFactory &fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    tp = fact.createStart(AbstractTaskFactory::START_LINE,*wp);
    if (tp->get_oz()->shape == ObservationZonePoint::CYLINDER) {
      CylinderZone *cz = (CylinderZone *)tp->get_oz();
      cz->setRadius(fixed(5000.0));
      tp->UpdateOZ(projection);
    }
    if (!fact.append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact.append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (tp->get_oz()->shape == ObservationZonePoint::CYLINDER) {
      CylinderZone *cz = (CylinderZone *)tp->get_oz();
      cz->setRadius(fixed(30000.0));
      tp->UpdateOZ(projection);
    }
    if (!fact.append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(4);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (!fact.append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(5);
  if (wp) {
    tp = fact.createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (tp->get_oz()->shape == ObservationZonePoint::CYLINDER) {
      CylinderZone *cz = (CylinderZone *)tp->get_oz();
      cz->setRadius(fixed(30000.0));
      tp->UpdateOZ(projection);
    }
    if (!fact.append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    tp = fact.createFinish(AbstractTaskFactory::FINISH_LINE,*wp);
    if (!fact.append(*tp,false)) return false;
    delete tp;
  } else {
    return false;
  }

  task_report(task_manager, "# checking task\n");
  if (!fact.validate()) {
    return false;
  }

  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;
}


bool test_task_fai(TaskManager& task_manager,
                   const Waypoints &waypoints)
{
  task_manager.set_factory(TaskBehaviour::FACTORY_FAI_GENERAL);
  AbstractTaskFactory &fact = task_manager.get_factory();
  const Waypoint *wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createStart(*wp);
    if (!fact.append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    OrderedTaskPoint *tp = fact.createIntermediate(*wp);
    if (!fact.append(*tp, false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    OrderedTaskPoint *tp = fact.createIntermediate(*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createFinish(*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# checking task\n");
  if (!fact.validate()) {
    return false;
  }

  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;
}


bool test_task_aat(TaskManager& task_manager,
                   const Waypoints &waypoints)
{
  const TaskProjection &projection =
    task_manager.get_ordered_task().get_task_projection();

  task_manager.set_factory(TaskBehaviour::FACTORY_AAT);
  AbstractTaskFactory &fact = task_manager.get_factory();
  const Waypoint *wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createStart(*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    OrderedTaskPoint* tp = fact.createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (tp->get_oz()->shape == ObservationZonePoint::CYLINDER) {
      CylinderZone *cz = (CylinderZone *)tp->get_oz();
      cz->setRadius(fixed(30000.0));
      tp->UpdateOZ(projection);
    }
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    OrderedTaskPoint* tp = fact.createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (tp->get_oz()->shape == ObservationZonePoint::CYLINDER) {
      CylinderZone *cz = (CylinderZone *)tp->get_oz();
      cz->setRadius(fixed(40000.0));
      tp->UpdateOZ(projection);
    }
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createFinish(*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact.validate()) {
    return false;
  }

  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;
}


bool test_task_or(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  const Waypoint *wp;

  task_manager.set_factory(TaskBehaviour::FACTORY_FAI_OR);
  AbstractTaskFactory &fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createStart(*wp);
    if (!fact.append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    OrderedTaskPoint *tp = fact.createIntermediate(*wp);
    if (!fact.append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createFinish(*wp);
    if (!fact.append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact.validate()) {
    return false;
  }

  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;

}


bool test_task_dash(TaskManager& task_manager,
                    const Waypoints &waypoints)
{
  const Waypoint *wp;

  task_manager.set_factory(TaskBehaviour::FACTORY_TOURING);
  AbstractTaskFactory &fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createStart(*wp);
    if (!fact.append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    OrderedTaskPoint *tp = fact.createFinish(*wp);
    if (!fact.append(*tp)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact.validate()) {
    return false;
  }

  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;

}


bool test_task_fg(TaskManager& task_manager,
                  const Waypoints &waypoints)
{
  const Waypoint *wp;

  task_manager.set_factory(TaskBehaviour::FACTORY_FAI_GOAL);
  AbstractTaskFactory &fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    OrderedTaskPoint *tp = fact.createStart(*wp);
    if (!fact.append(*tp, false)) {
      return false;
    }
    delete tp;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(6);
  if (wp) {
    OrderedTaskPoint *tp = fact.createFinish(*wp);
    if (!fact.append(*tp, false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact.validate()) {
    return false;
  }

  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;
}


const Waypoint* random_waypoint(const Waypoints &waypoints) {
  static unsigned id_last = 0;
  unsigned id = 0;
  do {
    id = rand() % waypoints.size()+1;
  } while (id==id_last);
  id_last = id;
  return waypoints.lookup_id(id);  
}

bool test_task_random(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned num_points)
{
  const Waypoint *wp;

  OrderedTaskPoint *tp;

  task_manager.set_factory(TaskBehaviour::FACTORY_MIXED);
  AbstractTaskFactory &fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    AbstractTaskFactory::LegalPointType_t s = 
      fact.getStartTypes()[(rand() % fact.getStartTypes().size())];

    tp = fact.createStart(s,*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  for (unsigned i=0; i<num_points; i++) {
    task_report(task_manager, "# adding intermediate\n");
    wp = random_waypoint(waypoints);
    if (wp) {
      AbstractTaskFactory::LegalPointType_t s = 
        fact.getIntermediateTypes()[(rand() % fact.getIntermediateTypes().size())];

      tp = fact.createIntermediate(s,*wp);
      if (!fact.append(*tp,false)) {
        return false;
      }
      delete tp;
    }
  }

  task_report(task_manager, "# adding finish\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    AbstractTaskFactory::LegalPointType_t s = 
      fact.getFinishTypes()[(rand() % fact.getFinishTypes().size())];

    tp = fact.createFinish(s,*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  task_report(task_manager, "# validating task..\n");
  if (!fact.validate()) {
    return false;
  }
  task_report(task_manager, "# checking task..\n");
  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;
}

bool test_task_random_RT_AAT_FAI(TaskManager& task_manager,
                      const Waypoints &waypoints,
                      const unsigned _num_points)
{
  const Waypoint *wp;

  OrderedTaskPoint *tp;
  AbstractTaskFactory &fact = task_manager.get_factory();
  char tmp[255];
  char tskType[20];
  tskType[0] = '\0';


  switch (rand() %3) {
  case 0:
    task_manager.set_factory(TaskBehaviour::FACTORY_AAT);
    strcpy(tskType,"AAT");
    test_note("# creating random AAT task\n");
    break;
  case 1:
    task_manager.set_factory(TaskBehaviour::FACTORY_RT);
    strcpy(tskType,"RT");
    test_note("# creating random RT task\n");
    break;
  case 2:
    task_manager.set_factory(TaskBehaviour::FACTORY_FAI_GENERAL);
    strcpy(tskType,"FAI");
    test_note("# creating random FAI GENERAL\n");
    break;
  }

  //max points includes start & finish
  const unsigned num_points_total = (
    max(task_manager.get_ordered_task_behaviour().min_points,
        (_num_points % task_manager.get_ordered_task_behaviour().max_points) + 1));
  const unsigned num_int_points = num_points_total - 2;

  test_note("# adding start\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    AbstractTaskFactory::LegalPointType_t s =
      fact.getStartTypes()[(rand() % fact.getStartTypes().size())];

    tp = fact.createStart(s,*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  for (unsigned i=0; i<num_int_points; i++) {
    test_note("# adding intermediate\n");
    wp = random_waypoint(waypoints);
    if (wp) {
      AbstractTaskFactory::LegalPointType_t s =
        fact.getIntermediateTypes()[(rand() % fact.getIntermediateTypes().size())];

      tp = fact.createIntermediate(s,*wp);
      if (!fact.append(*tp,false)) {
        return false;
      }
      delete tp;
    }
  }

  test_note("# adding finish\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    AbstractTaskFactory::LegalPointType_t s =
      fact.getFinishTypes()[(rand() % fact.getFinishTypes().size())];

    tp = fact.createFinish(s,*wp);
    if (!fact.append(*tp,false)) {
      return false;
    }
    delete tp;
  }

  test_note("# validating task..\n");
  if (!fact.validate()) {
    return false;
  }
  if (task_manager.get_ordered_task().get_factory_type()
      == TaskBehaviour::FACTORY_FAI_GENERAL)
  {
    test_note("# checking OZs for FAI General..\n");
    if (!fact.validateFAIOZs())
      return false;
  }

  task_manager.resume();
  sprintf(tmp, "# SUCCESS CREATING %s task! task_size():%d..\n",
      tskType,
      task_manager.task_size());
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
  default:
    return "unknown";
    break;
  };
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
    break;
  };
}


