#include "TaskVario.hpp"
#include "GlideSolvers/GlideResult.hpp"


TaskVario::TaskVario():
  value(0.0),
  df(fixed_zero),
  v_lpf(fixed(120), false)
{

}

void 
TaskVario::update(const GlideResult& solution, const fixed dt)
{
  fixed v = df.update(solution.AltitudeDifference);
  value = v_lpf.update(v);
}

void 
TaskVario::reset(const GlideResult& solution)
{
  v_lpf.reset(fixed_zero);
  df.reset(solution.AltitudeDifference, fixed_zero);
  value = fixed_zero;
}
