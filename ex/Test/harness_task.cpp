#include "harness_task.hpp"
#include "test_debug.hpp"

#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/Visitors/TaskVisitor.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Task/Visitors/ObservationZoneVisitor.hpp"

#include "harness_waypoints.hpp"

class ObservationZoneVisitorPrint: public ObservationZoneVisitor
{
public:
  virtual void Visit(const SectorZone& oz) {
    printf("# sector zone\n");
  }
  virtual void Visit(const LineSectorZone& oz) {
    printf("# line zone\n");
  }
  virtual void Visit(const CylinderZone& oz) {
    printf("# cylinder zone\n");
  }
};

class TaskPointVisitorPrint: public TaskPointVisitor
{
public:
  virtual void Visit(const UnorderedTaskPoint& tp) {
    printf("# got a tp\n");
  }
  virtual void Visit(const OrderedTaskPoint& tp) {
    printf("# got an otp\n");
    tp.Accept_oz(ozv);
  }
  virtual void Visit(const FinishPoint& tp) {
    printf("# got an ftp\n");
    tp.Accept_oz(ozv);
  }
  virtual void Visit(const StartPoint& tp) {
    printf("# got an stp\n");
    tp.Accept_oz(ozv);
  }
  virtual void Visit(const AATPoint& tp) {
    printf("# got an aat\n");
    tp.Accept_oz(ozv);
  }
  virtual void Visit(const ASTPoint& tp) {
    printf("# got an ast\n");
    tp.Accept_oz(ozv);
  }
private:
  ObservationZoneVisitorPrint ozv;
};

class TaskVisitorPrint: public TaskVisitor
{
public:
  virtual void Visit(const AbortTask& task) {
    printf("# task is abort\n");
    task.Accept(tpv);
    print_distances(task);
  };
  virtual void Visit(const OrderedTask& task) {
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
    task.Accept(tpv);
    print_distances(task);
  };
  virtual void print_distances(const AbstractTask& task) {
    printf("# - dist nominal %g\n",task.get_stats().distance_nominal);
  };
private:
  TaskPointVisitorPrint tpv;
};

void task_report(TaskManager& task_manager, const char* text)
{
  AIRCRAFT_STATE ac;
  if (verbose) {
#ifdef DO_PRINT
    TaskVisitorPrint tv;
    printf("%s",text);
    task_manager.Accept(tv);
    task_manager.print(ac);
#endif
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
  if (fact->remove(0-1)) {
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

  task_report(task_manager, "# auto-replacing at 2 (no morph)\n");
  wp = waypoints.lookup_id(9);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact->replace(tp,2)) return false;
  }

  task_report(task_manager, "# auto-replacing at 2 (morph)\n");
  wp = waypoints.lookup_id(9);
  if (wp) {
    tp = fact->createStart(*wp);
    if (!fact->replace(tp,2)) return false;
  }

  task_report(task_manager, "# auto-replacing at 0 (morph this)\n");
  wp = waypoints.lookup_id(12);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::FAI_SECTOR,*wp);
    if (!fact->replace(tp,0)) return false;
  }

  task_report(task_manager, "# auto-replacing at end (morph this)\n");
  wp = waypoints.lookup_id(14);
  if (wp) {
    tp = fact->createIntermediate(AbstractTaskFactory::AST_CYLINDER,*wp);
    if (!fact->replace(tp,task_manager.task_size()-1)) return false;
  }

  task_report(task_manager, "# removing finish point\n");
  if (!fact->remove(task_manager.task_size()-1)) {
    return false;
  }

  task_report(task_manager, "# inserting at 50 (equivalent to append)\n");
  wp = waypoints.lookup_id(8);
  if (wp) {
    tp = fact->createFinish(*wp);
    if (!fact->insert(tp,50)) return false;
  }

  task_report(task_manager, "# inserting at 0 (morph this)\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    tp = fact->createFinish(*wp);
    if (!fact->insert(tp,0)) return false;
  }

  task_report(task_manager, "# inserting at 2 (morph this)\n");
  wp = waypoints.lookup_id(4);
  if (wp) {
    tp = fact->createStart(*wp);
    if (!fact->insert(tp,2)) return false;
  }

  task_report(task_manager, "# inserting at 2 (direct)\n");
  wp = waypoints.lookup_id(6);
  if (wp) {
    tp = fact->createIntermediate(*wp);
    if (!fact->insert(tp,2,false)) return false;
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
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(30000.0);
      tp->update_oz();
    }
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

  if (!task_manager.check_ordered_task()) {
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

  if (!task_manager.check_ordered_task()) {
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
    OrderedTaskPoint* tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(30000.0);
      tp->update_oz();
    }
    if (!fact->append(tp,false)) {
      return false;
    }
  }

  task_report(task_manager, "# adding intermediate\n");
  wp = waypoints.lookup_id(3);
  if (wp) {
    OrderedTaskPoint* tp = fact->createIntermediate(AbstractTaskFactory::AAT_CYLINDER,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(40000.0);
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

  if (!task_manager.check_ordered_task()) {
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

  if (!task_manager.check_ordered_task()) {
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
  wp = waypoints.lookup_id(3);
  if (wp) {
    if (!fact->append(fact->createFinish(*wp))) {
      return false;
    }
  }

  task_report(task_manager, "# checking task..\n");
  if (!fact->validate()) {
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
  AbstractTaskFactory *fact;
  const Waypoint *wp;

  OrderedTaskPoint *tp;

  task_manager.set_factory(TaskManager::FACTORY_MIXED);
  fact = task_manager.get_factory();

  task_report(task_manager, "# adding start\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    AbstractTaskFactory::LegalStartType_t s = 
      fact->getStartTypes()[(rand() % fact->getStartTypes().size())];

    tp = fact->createStart(s,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(500.0);
      tp->update_oz();
    }
    if (!fact->append(tp,false)) {
      return false;
    }
  }

  task_manager.setActiveTaskPoint(0);
  task_manager.resume();

  for (unsigned i=0; i<num_points; i++) {
    task_report(task_manager, "# adding intermediate\n");
    wp = random_waypoint(waypoints);
    if (wp) {
      AbstractTaskFactory::LegalIntermediateType_t s = 
        fact->getIntermediateTypes()[(rand() % fact->getIntermediateTypes().size())];

      tp = fact->createIntermediate(s,*wp);
      if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
        cz->setRadius(500.0);
        tp->update_oz();
      }
      if (!fact->append(tp,false)) {
        return false;
      }
    }
  }

  task_report(task_manager, "# adding finish\n");
  wp = random_waypoint(waypoints);
  if (wp) {
    AbstractTaskFactory::LegalFinishType_t s = 
      fact->getFinishTypes()[(rand() % fact->getFinishTypes().size())];

    tp = fact->createFinish(s,*wp);
    if (CylinderZone* cz = dynamic_cast<CylinderZone*>(tp->get_oz())) {
      cz->setRadius(500.0);
      tp->update_oz();
    }
    if (!fact->append(tp,false)) {
      return false;
    }
  }

  task_report(task_manager, "# validating task..\n");
  if (!fact->validate()) {
    return false;
  }
  task_report(task_manager, "# checking task..\n");
  if (!task_manager.check_ordered_task()) {
    return false;
  }
  return true;
}


bool test_task(TaskManager& task_manager,
               const Waypoints &waypoints,
               int test_num)
{
  unsigned n_points = rand()%8;
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


