#include "TaskVario.hpp"
#include "GlideSolvers/GlideResult.hpp"

TaskVarioComputer::TaskVarioComputer()
  :df(fixed(0)),
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
  v_lpf.Reset(fixed(0));
  df.Reset(solution.altitude_difference, fixed(0));
  data.value = fixed(0);
}
