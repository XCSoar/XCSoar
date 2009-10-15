#include "GlideSolvers/MacCready.hpp"
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Util/Quadratic.hpp"
#include "Navigation/Aircraft.hpp"
#include "Math/Geometry.hpp"
#include "Math/NavFunctions.hpp"

long count_mc = 0;

bool GLIDE_RESULT::superior(const GLIDE_RESULT &s2) const 
{
  if (Solution < s2.Solution) {
    return true;
  } else if (ok_or_partial() && (Solution == s2.Solution)) {
    if (Distance>0) {
      return (Distance/(TimeElapsed+TimeVirtual) > 
              s2.Distance/(s2.TimeElapsed+s2.TimeVirtual));
    } else {
      return (TimeElapsed+TimeVirtual < s2.TimeElapsed+s2.TimeVirtual);
    }
  } else {
    return false;
  }
}

void GLIDE_RESULT::print(std::ostream& f) const
{
  if (Solution != MacCready::RESULT_OK) {
    f << "#     Solution NOT OK\n";
  }
  f << "#    Altitude Difference " << AltitudeDifference << "\n";
  f << "#    Distance            " << Distance << "\n";
  f << "#    TrackBearing        " << TrackBearing << "\n";
  f << "#    CruiseTrackBearing  " <<  CruiseTrackBearing << "\n";
  f << "#    VOpt                " <<  VOpt << "\n";
  f << "#    HeightClimb         " <<  HeightClimb << "\n";
  f << "#    HeightGlide         " <<  HeightGlide << "\n";
  f << "#    TimeElapsed         " <<  TimeElapsed << "\n";
  f << "#    TimeVirtual         " <<  TimeVirtual << "\n";
  if (TimeElapsed>0) {
  f << "#    Vave remaining      " <<  Distance/TimeElapsed << "\n";
  }
}

void GLIDE_RESULT::add(const GLIDE_RESULT &s2) 
{
  TimeElapsed += s2.TimeElapsed;
  HeightGlide += s2.HeightGlide;
  HeightClimb += s2.HeightClimb;
  Distance    += s2.Distance;
  TimeVirtual += s2.TimeVirtual;
  if ((AltitudeDifference>0) && (s2.AltitudeDifference>0)) {
    AltitudeDifference= std::max(AltitudeDifference, s2.AltitudeDifference);
  } else {
    // add differences if both under
    AltitudeDifference+= std::min(s2.AltitudeDifference, 0.0);
  }
}

double MacCready::SinkRate(const double V) const
{
  const double dV = (V-25.0)/25.0;
  return 0.5+dV*dV*2.0;
}

GLIDE_RESULT MacCready::solve_vertical(const AIRCRAFT_STATE &aircraft,
                                       const GLIDE_STATE &task) const
{
  // TODO this equation needs to be checked

  const double W = aircraft.WindSpeed;
  const double dh = task.MinHeight-aircraft.Altitude;
  const double V = VOpt*cruise_efficiency;
//  double S = SinkRate(VOpt);

  GLIDE_RESULT result;
  result.TrackBearing = task.Bearing;
  result.CruiseTrackBearing = -aircraft.WindDirection;
  result.VOpt = VOpt;
  result.Distance = task.Distance;
  result.HeightClimb = 0;
  result.HeightGlide = 0;
  result.TimeElapsed = 0;
  result.AltitudeDifference = -dh;

  // distance relation
  //   V*t_cr = W*(t_cl+t_cr)
  //     t_cr*(V-W)=W*t_cl
  //     t_cr = (W*t_cl)/(V-W)     .... (1)

  // height relation
  //   t_cl = (dh+t_cr*S)/mc
  //     t_cl*mc = (dh+(W*t_cl)/(V-W))    substitute (1)
  //     t_cl*mc*(V-W)= dh*(V-W)+W*t_cl
  //     t_cl*(mc*(V-W)-W) = dh*(V-W) .... (2)

  if (dh<0) {
    // immediate solution
    result.Solution = RESULT_OK;
    return result;
  }
  
  double denom1 = V-W;
  if (denom1<=0) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }

  double denom2 = mc*(V-W)-W;
  if (denom2<=0) {
    result.Solution = RESULT_MACCREADY_INSUFFICIENT;
    return result;
  } 
    
  const double t_cl = dh*(V-W)/denom2; // from (2)
  const double t_cr = (W*t_cl)/denom1; // from (1)

  result.TimeElapsed = t_cr+t_cl;
  result.HeightClimb = dh;
  result.HeightGlide = 0;
  result.Solution = RESULT_OK;
  result.AltitudeDifference = 0;

  return result;
}


double MacCready::cruise_bearing(const double V, 
                                 const double Wn, 
                                 const double theta) const
{
  if (Wn==0.0) {
    return 0.0;
  }
  const double sintheta = sin(theta);
  if (sintheta==0.0) {
    return 0.0;
  }
  // Wn/sin(alpha) = V/sin(theta)
  //   (Wn/V)*sin(theta) = sin(alpha)

  const double alpha = asin(sintheta*Wn/V);
  return RAD_TO_DEG*alpha;
}


GLIDE_RESULT MacCready::solve_glide(const AIRCRAFT_STATE &aircraft,
                                    const GLIDE_STATE &task,
                                    const double Vset) const
{
  // spend a lot of time in this function, so it should be quick!

  count_mc++;

  const double V = Vset*cruise_efficiency;
  const double W = aircraft.WindSpeed;
  const double theta = aircraft.WindDirection-task.Bearing;
  const double dh = task.MinHeight-aircraft.Altitude;

  GLIDE_RESULT result;
  result.TrackBearing = task.Bearing;
  result.CruiseTrackBearing = task.Bearing;
  result.VOpt = Vset;
  result.Distance = 0;
  result.HeightClimb = 0;
  result.HeightGlide = 0;
  result.TimeElapsed = 0;
  result.AltitudeDifference = -dh;

  // distance relation
  //   V*V=Vn*Vn+W*W-2*Vn*W*cos(theta)
  //     Vn*Vn-2*Vn*W*cos(theta)+W*W-V*V=0  ... (1)

  const double costheta = cos(DEG_TO_RAD*(theta));
  const Quadratic q(1.0,2.0*W*costheta,W*W-V*V); // from (1)

  if (!q.check()) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }
  const double Vn = q.solution_max();
  if (Vn<=0.0) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }

  const double gamma = dh/task.Distance;
  const double S = SinkRate(Vset);

  if (gamma*Vn+S>0) {
    if (gamma>0) {
      // insufficient height, and can't climb
      result.Distance = 0;
      result.Solution = RESULT_MACCREADY_INSUFFICIENT;
      return result;
    } else {
      // frac = gamma/(-S/Vn)
      const double frac = gamma/(-S/Vn);
      result.Distance = frac*task.Distance;
      result.Solution = RESULT_PARTIAL;
    }
  } else {
    result.Distance = task.Distance;
    result.Solution = RESULT_OK;
  }

  const double t_cr = result.Distance/Vn;
  result.TimeElapsed = t_cr;
  result.HeightClimb = 0;
  result.HeightGlide = t_cr*S;
  result.CruiseTrackBearing = ::AngleLimit360(result.TrackBearing+
                                              cruise_bearing(Vset,W,theta));
  result.AltitudeDifference = -dh-result.HeightGlide;

  return result;
}


GLIDE_RESULT MacCready::solve_cruise(const AIRCRAFT_STATE &aircraft,
                                     const GLIDE_STATE &task) const
{
  const double W = aircraft.WindSpeed;
  const double dh = task.MinHeight-aircraft.Altitude;

  GLIDE_RESULT result;
  result.TrackBearing = task.Bearing;
  result.CruiseTrackBearing = task.Bearing;
  result.VOpt = VOpt;
  result.HeightClimb = 0;
  result.HeightGlide = 0;
  result.TimeElapsed = 0;
  result.Distance = 0;
  result.AltitudeDifference = -dh;

  const double V = VOpt*cruise_efficiency;
  const double S = SinkRate(VOpt);

  // distance relation
  const double rho = S/mc;
  const double k = (1+rho)*(1+rho);
  const double theta = aircraft.WindDirection-task.Bearing;
  const double costheta = cos(theta);

  const Quadratic q(k,
                    -2.0*k*W*costheta,
                    k*W*W-V*V); // from (1)

  if (!q.check()) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }
  const double Vn = q.solution_max();
  if (Vn<=0.0) {
    result.Solution = RESULT_MACCREADY_INSUFFICIENT;
    return result;
  }

  double t_cl1 = 0.0;
  double distance = task.Distance;
  if (dh>0) {
    t_cl1 = dh/mc;
    double wd = DEG_TO_RAD*(aircraft.WindDirection);
    double tb = DEG_TO_RAD*(task.Bearing);
    double dx= t_cl1*W*sin(wd)-task.Distance*sin(tb);
    double dy= t_cl1*W*cos(wd)-task.Distance*cos(tb);
    distance = sqrt(dx*dx+dy*dy);
//    task.Bearing = RAD_TO_DEG*(atan2(dx,dy));
  }

  const double t_cr = distance/Vn;
  double t_cl = t_cr*rho + (dh>0? t_cl1:0);

  result.TimeElapsed = t_cr+t_cl;
  result.HeightClimb = t_cl*mc;
  result.HeightGlide = t_cr*S-result.HeightClimb;
  result.Distance = task.Distance;
  result.AltitudeDifference = -dh+result.HeightClimb-result.HeightGlide;

  result.CruiseTrackBearing = 
    ::AngleLimit360(result.TrackBearing+
                    cruise_bearing(VOpt,W*(1+std::max(0.0,rho)),theta));

  result.Solution = RESULT_OK;

  return result;
}


GLIDE_RESULT MacCready::solve(const AIRCRAFT_STATE &aircraft,
                              const GLIDE_STATE &task) const
{

  count_mc++;

  if (task.Distance==0) {
    return solve_vertical(aircraft, task);
  } 
  if (mc==0) {
    return optimise_glide(aircraft, task);
  }

  // check first if can final glide
  GLIDE_RESULT result_fg = optimise_glide(aircraft, task);
  if (result_fg.Solution == RESULT_OK) {
    // whole task final glided
    return result_fg;
  }

  // climb-cruise remainder of way

  GLIDE_STATE sub_task = task;
  sub_task.Distance -= result_fg.Distance;
  sub_task.MinHeight += result_fg.HeightGlide;

  GLIDE_RESULT result_cc = solve_cruise(aircraft, sub_task);
  result_cc.add(result_fg);

  return result_cc;
}





#include "GlideSolvers/ZeroFinder.hpp"


class MacCreadyVopt: 
  public ZeroFinder
{
public:
  MacCreadyVopt(const AIRCRAFT_STATE &_aircraft,
                const GLIDE_STATE &_task,
                const MacCready &_mac):
    ZeroFinder(15.0,75.0,0.01),
    aircraft(_aircraft),
    task(_task),
    mac(_mac)
    {
    };
  virtual double f(const double V) {
    res = mac.solve_glide(aircraft, task, V);
    if (mac.get_mc()>0) {
      // equivalent time to gain the height that was used
      res.TimeVirtual = std::max(res.HeightGlide/mac.get_mc(),0.0);
    } else {
      res.TimeVirtual = 0.0;
    }
    if (res.Distance>0) {
      return (res.TimeElapsed+res.TimeVirtual)/res.Distance;
    } else {
      return 0.0;
    }
  }
  virtual GLIDE_RESULT result() {
    find_min(20.0);
    return res;
  }
  GLIDE_RESULT res;
protected:
  const AIRCRAFT_STATE &aircraft;
  const GLIDE_STATE &task;
  const MacCready &mac;
};


void MacCready::set_mc(double _mc)
{
  mc = _mc;
  solve_vopt();
}

void MacCready::solve_vopt()
{
  GLIDE_STATE task;
  task.Distance = 1.0;
  task.Bearing = 0;
  task.MinHeight = 0;
  AIRCRAFT_STATE aircraft;
  aircraft.WindSpeed = 0;
  aircraft.WindDirection = 0;
  aircraft.Altitude = 10000;
  MacCreadyVopt mcvopt(aircraft, task, *this);
  VOpt = mcvopt.find_min(20.0);
}


GLIDE_RESULT MacCready::optimise_glide(const AIRCRAFT_STATE &aircraft,
                                       const GLIDE_STATE &task) const
{
  MacCreadyVopt mcvopt(aircraft, task, *this);
  return mcvopt.result();
}





/*
  // distance relation

  (W*(1+rho))**2+((1+rho)*Vn)**2-2*W*(1+rho)*Vn*(1+rho)*costheta-V*V

subs rho=(gamma*Vn+S)/mc
-> rho = S/mc
   k = 1+rho = (S+mc)/mc
    rho = (S+M)/M-1

*/
