#include "GlidePolar.hpp"
#include "GlideState.hpp"
#include "GlideResult.hpp"
#include "MacCready.hpp"
#include "ZeroFinder.hpp"

GlidePolar::GlidePolar(const double _mc,
                       const double _bugs,
                       const double _ballast):
  mc(_mc),
  bugs(_bugs),
  ballast(_ballast),
  cruise_efficiency(1.0)
{
  solve();
}

void
GlidePolar::set_mc(const double _mc)
{
  mc = _mc;
  solve();
}

double
GlidePolar::MSinkRate(const double V) const
{
  return SinkRate(V)+mc;
}

double 
GlidePolar::SinkRate(const double V) const
{
  const double dV = (V-25.0)*0.056;
  return 0.5+(dV*dV+V*0.01)/2.0;
}


class GlidePolarVopt: 
  public ZeroFinder
{
public:
  GlidePolarVopt(const GlidePolar &_polar):
    ZeroFinder(15.0,75.0,0.01),
    polar(_polar)
    {
    };
  double f(const double V) {
    return -V/polar.MSinkRate(V);
  }
protected:
  const GlidePolar &polar;
};


void 
GlidePolar::solve()
{
  GlidePolarVopt gpvopt(*this);
  VbestLD = gpvopt.find_min(20.0);
  SbestLD = SinkRate(VbestLD);
}


long count_mc = 0;


GLIDE_RESULT 
GlidePolar::solve(const GLIDE_STATE &task) const
{
  count_mc++;
  MacCready mac(*this, cruise_efficiency);
  return mac.solve(task);
}

GLIDE_RESULT 
GlidePolar::solve_sink(const GLIDE_STATE &task,
                       const double S) const
{
  count_mc++;
  MacCready mac(*this, cruise_efficiency);
  return mac.solve_sink(task,S);
}
