#include "TaskEvents.hpp"
#include "Tasks/BaseTask/TaskPoint.hpp"

#ifdef DO_PRINT
#include <stdio.h>
#endif

void 
TaskEvents::transition_enter(const TaskPoint& tp) const
{
#ifdef DO_PRINT
  printf("- entered sector\n");
#endif
}

void 
TaskEvents::transition_exit(const TaskPoint &tp) const
{
#ifdef DO_PRINT
  printf("- exited sector\n");
#endif
}


void 
TaskEvents::active_advanced(const TaskPoint &tp, const int i) const
{
#ifdef DO_PRINT
  printf("- advance to sector %d\n", i);
#endif
}

void 
TaskEvents::active_changed(const TaskPoint &tp) const
{
#ifdef DO_PRINT
  printf("- active changed to wp %d\n", tp.get_waypoint().id);
#endif
}

void 
TaskEvents::construction_error(const char* error) const
{
#ifdef DO_PRINT
  printf("Task construction error: ");
  printf(error);
  printf("\n");
#endif
}
