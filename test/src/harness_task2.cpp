#include "harness_task.hpp"
#include "test_debug.hpp"
#include "harness_waypoints.hpp"


bool test_task_bad(TaskManager& task_manager,
                   const Waypoints& waypoints)
{
  test_task_random(task_manager,waypoints,2);

  task_manager.set_factory(OrderedTask::FACTORY_FAI);
  AbstractTaskFactory& fact = task_manager.get_factory();

  const Waypoint* wp = random_waypoint(waypoints);

  ok (!fact.createFinish((AbstractTaskFactory::LegalFinishType_t)10,*wp),"bad finish type",0);
  ok (!fact.createStart((AbstractTaskFactory::LegalStartType_t)10,*wp),"bad start type",0);
  ok (!fact.createIntermediate((AbstractTaskFactory::LegalIntermediateType_t)10,*wp),"bad intermediate type",0);

  // now create a taskpoint from FAI

  AbstractTaskFactory::LegalIntermediateType_t s = 
    fact.getIntermediateTypes()[(rand() % fact.getIntermediateTypes().size())];

  // test it is bad for AAT

  task_manager.set_factory(OrderedTask::FACTORY_AAT);

  AbstractTaskFactory& bfact = task_manager.get_factory();

  ok (!bfact.createIntermediate(s,*wp),"bad intermediate type (after task change)",0);

  bfact.remove(1);
  ok (bfact.validate(),"ok with one tp",0);

  bfact.remove(1);
  ok (!bfact.validate(),"insufficient tps for aat",0);

  ok (bfact.remove(task_manager.task_size()-1,false),"remove finish manually",0);
  ok (!bfact.validate(),"aat is invalid (no finish)",0);

  return true;
}
