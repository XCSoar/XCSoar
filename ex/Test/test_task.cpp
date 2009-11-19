#include "test_task.hpp"
#include "test_debug.hpp"

#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"

#include "test_waypoints.hpp"

class TaskPointVisitorPrint: public TaskPointVisitor
{
public:
  virtual void Visit(const TaskPoint& tp) {
    printf("got a tp\n");
  }
  virtual void Visit(const OrderedTaskPoint& tp) {
    printf("got an otp\n");
  }
  virtual void Visit(const FinishPoint& tp) {
    printf("got an ftp\n");
  }
  virtual void Visit(const StartPoint& tp) {
    printf("got an stp\n");
  }
  virtual void Visit(const AATPoint& tp) {
    printf("got an aat\n");
  }
  virtual void Visit(const ASTPoint& tp) {
    printf("got an ast\n");
  }
};

class TaskVisitorPrint: public TaskVisitor
{
public:
  virtual void Visit(const AbortTask& task) {
    TaskPointVisitorPrint tpv;
    printf("task is abort\n");
    task.Accept(tpv);
  };
  virtual void Visit(const OrderedTask& task) {
    TaskPointVisitorPrint tpv;
    printf("task is ordered\n");
    task.Accept(tpv);
  };
  virtual void Visit(const GotoTask& task) {
    printf("task is goto\n");
  };
};


bool test_task(TaskManager& task_manager, const Waypoints &waypoints)
{
  AIRCRAFT_STATE ac;
//  AbstractTaskFactory *fact = task_manager.get_factory();

  TaskVisitorPrint tv;

  task_manager.Accept(tv);
  task_manager.print(ac);

  printf("removing tp 2\n");
  wait_prompt(0);
  if (!task_manager.remove(2)) {
    printf("can't remove 2\n");
    return false;
  }
  task_manager.Accept(tv);
  task_manager.print(ac);

  printf("removing tp 0\n");
  wait_prompt(0);
  if (!task_manager.remove(0)) {
    printf("can't remove 0\n");
    return false;
  }
  task_manager.Accept(tv);
  task_manager.print(ac);

  return true;
}

bool setup_task(TaskManager& task_manager, const Waypoints& waypoints)
{
  AbstractTaskFactory *fact;
  OrderedTaskPoint *tp;
  const Waypoint *wp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  wp = waypoints.lookup_id(1);
  if (wp) {
    tp = fact->createStart(AbstractTaskFactory::START_LINE,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(5000.0);
      tp->update_oz();
    }
    if (tp) task_manager.append(tp); else return false;
  } else {
    return false;
  }

  wp = waypoints.lookup_id(2);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::FAI_SECTOR,*wp);
    if (tp) task_manager.append(tp); else return false;
  }

  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(20000.0);
      tp->update_oz();
    }
    if (tp) task_manager.append(tp); else return false;
  }

  wp = waypoints.lookup_id(4);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (tp) task_manager.append(tp); else return false;
  }

  wp = waypoints.lookup_id(5);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (tp) task_manager.append(tp); else return false;
  }

  wp = waypoints.lookup_id(1);
  if (wp) {
    tp = fact->createFinish(AbstractTaskFactory::FINISH_SECTOR,*wp);
    if (tp) task_manager.append(tp); else return false;
  }

  if (task_manager.check_task()) {
    task_manager.reset();
    task_manager.setActiveTaskPoint(0);
    task_manager.resume();
  } else {
    return false;
  }

  TaskVisitorPrint tv;
  task_manager.Accept(tv);

  return true;
}
