#ifndef TASK_VARIO_HPP
#define TASK_VARIO_HPP

#include "Math/fixed.hpp"

#include <type_traits>

/**
 * Helper class to produce pseudo variometer based on rate of change
 * of task altitude difference.
 */
class TaskVario
{
  friend class TaskVarioComputer;

  fixed value;

public:
  void Reset() {
    value = fixed(0);
  }

/** 
 * Retrieve current vario value from last update
 * 
 * @return Current vario value (m/s, positive up)
 */
  fixed get_value() const {
    return value;
  }
};

static_assert(std::is_trivial<TaskVario>::value, "type is not trivial");

#endif
