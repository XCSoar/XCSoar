#include "AbstractTaskFactory.hpp"

#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"

StartPoint* 
AbstractTaskFactory::createStart(const LegalStartType_t type,
                                 const Waypoint &wp) const
{
  switch (type) {
  case START_SECTOR:
    return new StartPoint(new FAISectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case START_LINE:
    return new StartPoint(new LineSectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case START_CYLINDER:
    return new StartPoint(new CylinderZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  default:
    assert(1);
  };
  return NULL;
}

IntermediatePoint* 
AbstractTaskFactory::createIntermediate(const LegalIntermediateType_t type,
                                        const Waypoint &wp) const
{
  switch (type) {
  case FAI_SECTOR:
    return new ASTPoint(new FAISectorZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  case AST_CYLINDER:
    return new ASTPoint(new CylinderZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  case AAT_CYLINDER:
    return new AATPoint(new CylinderZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  case AAT_SEGMENT:
    return new AATPoint(new SectorZone(wp.Location),
                        task.get_task_projection(),wp,behaviour);
    break;
  default:
    assert(1);
  };
  return NULL;
}

FinishPoint* 
AbstractTaskFactory::createFinish(const LegalFinishType_t type,
                                 const Waypoint &wp) const
{
  switch (type) {
  case FINISH_SECTOR:
    return new FinishPoint(new FAISectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case FINISH_LINE:
    return new FinishPoint(new LineSectorZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  case FINISH_CYLINDER:
    return new FinishPoint(new CylinderZone(wp.Location),
                          task.get_task_projection(),wp,behaviour);
    break;
  default:
    assert(1);
  };
  return NULL;
}
