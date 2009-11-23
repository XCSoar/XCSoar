#include "harness_task.hpp"
#include "test_debug.hpp"

#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"

#include "harness_waypoints.hpp"

class TaskPointVisitorPrint: public TaskPointVisitor
{
public:
  virtual void Visit(const TaskPoint& tp) {
    printf("# got a tp\n");
  }
  virtual void Visit(const OrderedTaskPoint& tp) {
    printf("# got an otp\n");
  }
  virtual void Visit(const FinishPoint& tp) {
    printf("# got an ftp\n");
  }
  virtual void Visit(const StartPoint& tp) {
    printf("# got an stp\n");
  }
  virtual void Visit(const AATPoint& tp) {
    printf("# got an aat\n");
  }
  virtual void Visit(const ASTPoint& tp) {
    printf("# got an ast\n");
  }
};

class TaskVisitorPrint: public TaskVisitor
{
public:
  virtual void Visit(const AbortTask& task) {
    TaskPointVisitorPrint tpv;
    printf("# task is abort\n");
    task.Accept(tpv);
    print_distances(task);
  };
  virtual void Visit(const OrderedTask& task) {
    TaskPointVisitorPrint tpv;
    printf("# task is ordered\n");
    task.Accept(tpv);
    print_distances(task);
    if (task.get_stats().distance_max>task.get_stats().distance_min) {
      printf("# - dist max %g\n",task.get_stats().distance_max);
      printf("# - dist min %g\n",task.get_stats().distance_min);
    }
  };
  virtual void Visit(const GotoTask& task) {
    printf("# task is goto\n");
    print_distances(task);
  };
  virtual void print_distances(const AbstractTask& task) {
    printf("# - dist nominal %g\n",task.get_stats().distance_nominal);
  };
};

void task_report(TaskManager& task_manager, const char* text)
{
  AIRCRAFT_STATE ac;
  if (verbose) {
    TaskVisitorPrint tv;
    printf("%s",text);
    task_manager.Accept(tv);
    task_manager.print(ac);
  }
  if (interactive>1) {
    wait_prompt(0);
  }
}


bool test_task_manip(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  if (!test_task_mixed(task_manager, waypoints)) {
    return false;
  }
  AbstractTaskFactory *fact = task_manager.get_factory();

  task_report(task_manager, "# removing tp 2\n");
  if (!fact->remove(2)) {
    return false;
  }

  task_report(task_manager, "# removing tp 0\n");
  if (!fact->remove(0)) {
    return false;
  }

  task_report(task_manager, "# removing tp -1 (illegal)\n");
  if (fact->remove(-1)) {
    return false;
  }

  task_report(task_manager, "# removing tp 50 (illegal)\n");
  if (fact->remove(50)) {
    return false;
  }

  OrderedTaskPoint *tp;
  const Waypoint *wp;

  task_report(task_manager, "# inserting at 3\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact->insert(tp,3)) return false;
  }

  task_report(task_manager, "# removing finish point\n");
  if (!fact->remove(task_manager.get_task_size()-1)) {
    return false;
  }

  task_report(task_manager, "# inserting at 50 (equivalent to append)\n");
  wp = waypoints.lookup_id(8);
  if (wp) {
    tp = fact->createFinish(*wp);
    if (!fact->insert(tp,50)) return false;
  }

  task_report(task_manager, "# checking task\n");

  if (task_manager.check_task()) {
    task_manager.reset();
    task_manager.setActiveTaskPoint(0);
    task_manager.resume();
  } else {
    return false;
  }
  return true;
}


bool test_task_mixed(TaskManager& task_manager,
                     const Waypoints &waypoints)
{

  AbstractTaskFactory *fact;
  OrderedTaskPoint *tp;
  const Waypoint *wp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    tp = fact->createStart(AbstractTaskFactory::START_LINE,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(5000.0);
      tp->update_oz();
    }
    if (!fact->append(tp,false)) return false;
  } else {
    return false;
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::FAI_SECTOR,*wp);
    if (!fact->append(tp,false)) return false;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(30000.0);
      tp->update_oz();
    }
    if (!fact->append(tp,false)) return false;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(4);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (!fact->append(tp,false)) return false;
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(5);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (!fact->append(tp,false)) return false;
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    tp = fact->createFinish(AbstractTaskFactory::FINISH_SECTOR,*wp);
    if (!fact->append(tp,false)) return false;
  }

  task_report(task_manager, "# checking task\n");
  if (!fact->validate()) {
    return false;
  }

  if (!task_manager.check_task()) {
    return false;
  }
  return true;
}


bool test_task_fai(TaskManager& task_manager,
                   const Waypoints &waypoints)
{
  task_manager.set_factory(TaskManager::FACTORY_FAI);
  AbstractTaskFactory *fact = task_manager.get_factory();
  const Waypoint *wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createStart(*wp))) {
      return false;
    }
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    if (!fact->append(fact->createIntermediate(*wp),false)) {
      return false;
    }
  }

  task_report(task_manager, "# adding intermdiate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    if (!fact->append(fact->createIntermediate(*wp),false)) {
      return false;
    }
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createFinish(*wp),false)) {
      return false;
    }
  }

  task_report(task_manager, "# checking task\n");
  if (!fact->validate()) {
    return false;
  }

  if (!task_manager.check_task()) {
    return false;
  }
  return true;
}


bool test_task_aat(TaskManager& task_manager,
                   const Waypoints &waypoints)
{
  task_manager.set_factory(TaskManager::FACTORY_AAT);
  AbstractTaskFactory *fact = task_manager.get_factory();
  const Waypoint *wp;

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createStart(*wp),false)) {
      return false;
    }
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    if (!fact->append(fact->createIntermediate(*wp),false)) {
      return false;
    }
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    OrderedTaskPoint* tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(30000.0);
      tp->update_oz();
    }
    if (!fact->append(tp,false)) {
      return false;
    }
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createFinish(*wp),false)) {
      return false;
    }
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact->validate()) {
    return false;
  }

  if (!task_manager.check_task()) {
    return false;
  }
  return true;
}


bool test_task_or(TaskManager& task_manager,
                     const Waypoints &waypoints)
{
  AbstractTaskFactory *fact;
  const Waypoint *wp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createStart(*wp))) {
      return false;
    }
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    if (!fact->append(fact->createIntermediate(*wp))) {
      return false;
    }
  }

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createFinish(*wp))) {
      return false;
    }
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact->validate()) {
    return false;
  }

  if (!task_manager.check_task()) {
    return false;
  }
  return true;

}


bool test_task_dash(TaskManager& task_manager,
                    const Waypoints &waypoints)
{
  AbstractTaskFactory *fact;
  const Waypoint *wp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createStart(*wp))) {
      return false;
    }
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(2);
  if (wp) {
    if (!fact->append(fact->createFinish(*wp))) {
      return false;
    }
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact->validate()) {
    return false;
  }

  if (!task_manager.check_task()) {
    return false;
  }
  return true;

}


bool test_task_fg(TaskManager& task_manager,
                  const Waypoints &waypoints)
{
  AbstractTaskFactory *fact;
  const Waypoint *wp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = waypoints.lookup_id(1);
  if (wp) {
    if (!fact->append(fact->createStart(*wp))) {
      return false;
    }
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  task_report(task_manager, "# adding finish\n");
  wp = waypoints.lookup_id(6);
  if (wp) {
    if (!fact->append(fact->createFinish(*wp))) {
      return false;
    }
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact->validate()) {
    return false;
  }

  if (!task_manager.check_task()) {
    return false;
  }
  return true;

}


bool test_task(TaskManager& task_manager,
               const Waypoints &waypoints,
               int test_num)
{
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
  default:
    return "unknown";
    break;
  };
}
