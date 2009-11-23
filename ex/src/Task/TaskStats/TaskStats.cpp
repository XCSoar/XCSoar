#include "TaskStats.hpp"
#include <assert.h>

void
TaskStats::reset()
{
  total.reset();
  current_leg.reset();
}
