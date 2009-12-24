#include "TaskVario.hpp"
#include "GlideSolvers/GlideResult.hpp"


TaskVario::TaskVario():
  value(0.0),
  df(0.0),
  v_lpf(120.0,false)
{

}


double 
TaskVario::get_value() const
{
  return value;
}


void 
TaskVario::update(const GlideResult& solution, const fixed dt)
{
  double v = df.update(solution.AltitudeDifference);
  value = v_lpf.update(v);
}

void 
TaskVario::reset(const GlideResult& solution)
{
  v_lpf.reset(0.0);
  df.reset(solution.AltitudeDifference, 0.0);
}
