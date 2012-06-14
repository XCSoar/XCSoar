#include "TaskVario.hpp"
#include "GlideSolvers/GlideResult.hpp"

TaskVarioComputer::TaskVarioComputer()
  :df(fixed_zero),
   v_lpf(fixed(120), false)
{
}

void 
TaskVarioComputer::update(TaskVario &data, const GlideResult &solution,
                          const fixed dt)
{
  fixed v = df.Update(solution.altitude_difference);
  data.value = v_lpf.Update(v);
}

void 
TaskVarioComputer::reset(TaskVario &data, const GlideResult& solution)
{
  v_lpf.Reset(fixed_zero);
  df.Reset(solution.altitude_difference, fixed_zero);
  data.value = fixed_zero;
}
