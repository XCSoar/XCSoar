#include "GlidePolar.hpp"
#include "GlideState.hpp"
#include "GlideResult.hpp"
#include "MacCready.hpp"
#include "Util/ZeroFinder.hpp"
#include "Util/Tolerances.hpp"

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
  /// \todo note this is hardcoded at present, will need proper polar management later
  const double dV = (V-25.0)*0.056;
  return 0.5+(dV*dV+V*0.01)/2.0;
}

/**
 * Finds VOpt for a given MacCready setting
 */
class GlidePolarVopt: 
  public ZeroFinder
{
public:
  GlidePolarVopt(const GlidePolar &_polar):
    ZeroFinder(15.0, 75.0, TOLERANCE_POLAR_BESTLD),
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


#ifdef INSTRUMENT_TASK
long count_mc = 0;
#endif

GlideResult 
GlidePolar::solve(const GlideState &task) const
{
#ifdef INSTRUMENT_TASK
  count_mc++;
#endif
  MacCready mac(*this, cruise_efficiency);
  return mac.solve(task);
}

GlideResult 
GlidePolar::solve_sink(const GlideState &task,
                       const double S) const
{
#ifdef INSTRUMENT_TASK
  count_mc++;
#endif
  MacCready mac(*this, cruise_efficiency);
  return mac.solve_sink(task,S);
}


bool 
GlidePolar::possible_glide(const GlideState &task) const
{
  if (task.AltitudeDifference<=0) {
    return false;
  }
  // broad test assuming tailwind (best case)
  if ((VbestLD+task.EffectiveWindSpeed)*task.AltitudeDifference 
      < task.Vector.Distance*SbestLD) {
    return false;
  } else {
    return true;
  }
}

