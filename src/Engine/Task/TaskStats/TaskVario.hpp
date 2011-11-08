#ifndef TASK_VARIO_HPP
#define TASK_VARIO_HPP

#include "Math/fixed.hpp"
#include "Util/Filter.hpp"
#include "Util/DiffFilter.hpp"
#include "Util/TypeTraits.hpp"

struct GlideResult;

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
    value = fixed_zero;
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

static_assert(is_trivial<TaskVario>::value, "type is not trivial");

class TaskVarioComputer
{
  DiffFilter df;
  Filter v_lpf;

private:
  bool initialised;

public:
  TaskVarioComputer();

/** 
 * Update vario, taking altitude difference from a specified glide solution
 * 
 * @param solution Solution for task element
 * @param dt Time step
 */
  void update(TaskVario &data, const GlideResult &solution, const fixed dt);

/** 
 * Reset vario value (as if solution is held constant)
 * 
 * @param solution Element
 */
  void reset(TaskVario &data, const GlideResult &solution);
};

#endif
