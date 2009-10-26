#include "TaskEvents.hpp"
#include "BaseTask/TaskPoint.hpp"
#include <stdio.h>

void 
TaskEvents::transition_enter(const TaskPoint& tp) const
{
  printf("- entered sector\n");
}

void 
TaskEvents::transition_exit(const TaskPoint &tp) const
{
  printf("- exited sector\n");
}


void 
TaskEvents::active_advanced(const TaskPoint &tp, const int i) const
{
  printf("- advance to sector %d\n", i);
}

void 
TaskEvents::active_changed(const TaskPoint &tp) const
{
  printf("- active changed to wp %d\n", tp.get_waypoint().id);
}
