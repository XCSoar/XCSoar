#include "TaskEvents.hpp"
#include <stdio.h>

void 
TaskEvents::transition_enter(const TaskPoint& tp)
{
  printf("- entered sector\n");
}

void 
TaskEvents::transition_exit(const TaskPoint &tp)
{
  printf("- exited sector\n");
}


void 
TaskEvents::active_advanced(const TaskPoint &tp, const int i)
{
  printf("auto transition to sector %d\n", i);
}
