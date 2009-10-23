#include "GlideSolvers/MacCready.hpp"
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Util/Quadratic.hpp"
#include "Navigation/Aircraft.hpp"
#include "Math/Geometry.hpp"
#include "Math/NavFunctions.hpp"

long count_mc = 0;


void
GLIDE_RESULT::calc_cruise_bearing()
{
  CruiseTrackBearing=TrackBearing;
  if (EffectiveWindSpeed==0.0) {
    return;
  }
  const double sintheta = sin(DEG_TO_RAD*EffectiveWindAngle);
  if (sintheta==0.0) {
    return;
  }
  // Wn/sin(alpha) = V/sin(theta)
  //   (Wn/V)*sin(theta) = sin(alpha)
  CruiseTrackBearing += RAD_TO_DEG*asin(sintheta*EffectiveWindSpeed/VOpt);
}


GLIDE_RESULT::GLIDE_RESULT(const GLIDE_STATE &task, const double V):
    Solution(MacCready::RESULT_NOSOLUTION),
    Distance(task.Distance),
    TrackBearing(task.Bearing),
    CruiseTrackBearing(task.Bearing),
    VOpt(V),
    HeightClimb(0.0),
    HeightGlide(0.0),
    TimeElapsed(0.0),
    TimeVirtual(0.0),
    AltitudeDifference(0.0),
    EffectiveWindSpeed(0.0),
    EffectiveWindAngle(0.0)
{
}

/*
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
*/

void GLIDE_RESULT::print(std::ostream& f) const
{
  if (Solution != MacCready::RESULT_OK) {
    f << "#     Solution NOT OK\n";
  }
  f << "#    Altitude Difference " << AltitudeDifference << " (m)\n";
  f << "#    Distance            " << Distance << " (m)\n";
  f << "#    TrackBearing        " << TrackBearing << " (deg)\n";
  f << "#    CruiseTrackBearing  " <<  CruiseTrackBearing << " (deg)\n";
  f << "#    VOpt                " <<  VOpt << " (m/s)\n";
  f << "#    HeightClimb         " <<  HeightClimb << " (m)\n";
  f << "#    HeightGlide         " <<  HeightGlide << " (m)\n";
  f << "#    TimeElapsed         " <<  TimeElapsed << " (s)\n";
  f << "#    TimeVirtual         " <<  TimeVirtual << " (s)\n";
  if (TimeElapsed>0) {
  f << "#    Vave remaining      " <<  Distance/TimeElapsed << " (m/s)\n";
  f << "#    EffectiveWindSpeed  " <<  EffectiveWindSpeed << " (m/s)\n";
  f << "#    EffectiveWindAngle  " <<  EffectiveWindAngle << " (deg)\n";
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
  const double dV = (V-25.0)*0.056;
  return 0.5+dV*dV;
}

GLIDE_RESULT MacCready::solve_vertical(const AIRCRAFT_STATE &aircraft,
                                       const GLIDE_STATE &task) const
{
  // TODO this equation needs to be checked

  const double dh = task.MinHeight-aircraft.Altitude;
  const double W = aircraft.WindSpeed;
//  double S = SinkRate(VOpt);

  GLIDE_RESULT result(task,VOpt);
  result.AltitudeDifference = -dh;
  result.EffectiveWindSpeed = W;
  result.EffectiveWindAngle = aircraft.WindDirection-task.Bearing; // TODO check

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
  
  const double V = VOpt*cruise_efficiency;

  const double denom1 = V-W;
  if (denom1<=0) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }

  const double denom2 = mc*denom1-W;
  if (denom2<=0) {
    result.Solution = RESULT_MACCREADY_INSUFFICIENT;
    return result;
  } 
    
  const double t_cl = dh*denom1/denom2; // from (2)
  const double t_cr = (W*t_cl)/denom1; // from (1)

  result.TimeElapsed = t_cr+t_cl;
  result.HeightClimb = dh;
  result.HeightGlide = 0;
  result.Solution = RESULT_OK;
  result.AltitudeDifference = 0;

  return result;
}


GLIDE_RESULT MacCready::solve_glide(const AIRCRAFT_STATE &aircraft,
                                    const GLIDE_STATE &task,
                                    const double Vset) const
{
  const double S = SinkRate(Vset);
  return solve_glide(aircraft, task, Vset, S);
}


GLIDE_RESULT MacCready::solve_sink(const AIRCRAFT_STATE &aircraft,
                                   const GLIDE_STATE &task,
                                   const double S) const
{
  const double h_offset = 1.0e6;
  AIRCRAFT_STATE aircraft_virt = aircraft;
  aircraft_virt.Altitude += h_offset;
  GLIDE_RESULT res = solve_glide(aircraft_virt, task, VOpt, S);
  res.AltitudeDifference = aircraft.Altitude
    -res.HeightGlide-task.MinHeight;
  return res;
}


GLIDE_RESULT MacCready::solve_glide(const AIRCRAFT_STATE &aircraft,
                                    const GLIDE_STATE &task,
                                    const double Vset,
                                    const double S) const
{
  // spend a lot of time in this function, so it should be quick!

  count_mc++;

  const double V = Vset*cruise_efficiency;
  const double W = aircraft.WindSpeed;
  const double theta = aircraft.WindDirection-task.Bearing;
  const double dh = task.MinHeight-aircraft.Altitude;

  GLIDE_RESULT result(task,Vset);
  result.AltitudeDifference = -dh;
  result.EffectiveWindAngle = theta; 
  result.EffectiveWindSpeed = W;

  // distance relation
  //   V*V=Vn*Vn+W*W-2*Vn*W*cos(theta)
  //     Vn*Vn-2*Vn*W*cos(theta)+W*W-V*V=0  ... (1)

  const double costheta = cos(DEG_TO_RAD*(theta));
  const Quadratic q(2.0*W*costheta,W*W-V*V); // from (1)

  if (!q.check()) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    result.Distance = 0;
    return result;
  }
  const double Vn = q.solution_max();
  if (Vn<=0.0) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    result.Distance = 0;
    return result;
  }

  const double Vndh = Vn*dh;

  if (Vndh+S*task.Distance>0) { // Vn*dh+S*task.Distance>0
    if (dh>0) { 
      // insufficient height, and can't climb
      result.Distance = 0;
      result.Solution = RESULT_MACCREADY_INSUFFICIENT;
      return result;
    } else {
      // frac = -gammaVn/S // Vn*dh/(task.Distance*S)
      result.Distance = -Vndh/S; // frac*task.Distance; 
      result.Solution = RESULT_PARTIAL;
    }
  } else {
    result.Solution = RESULT_OK;
  }

  const double t_cr = result.Distance/Vn;
  result.TimeElapsed = t_cr;
  result.HeightGlide = t_cr*S;
  result.AltitudeDifference = -dh-result.HeightGlide;

  return result;
}


GLIDE_RESULT MacCready::solve_glide_zerowind(const AIRCRAFT_STATE &aircraft,
                                             const GLIDE_STATE &task,
                                             const double Vset) const
{
  // spend a lot of time in this function, so it should be quick!

  // this is ONLY used for finding Vopt for zero wind case, so makes assumptions
  // about solution, and don't bother calculating things we don't need

  count_mc++;

  const double S = SinkRate(Vset);
  const double Vn = Vset*cruise_efficiency;
  GLIDE_RESULT result(task,Vset);
  const double t_cr = result.Distance/Vn;
  result.Solution = RESULT_OK;
  result.TimeElapsed = t_cr;
  result.HeightGlide = t_cr*S;
  result.AltitudeDifference = aircraft.Altitude-result.HeightGlide;

  return result;
}


GLIDE_RESULT MacCready::solve_cruise(const AIRCRAFT_STATE &aircraft,
                                     const GLIDE_STATE &task) const
{
  const double W = aircraft.WindSpeed;
  const double dh = task.MinHeight-aircraft.Altitude;
  const double theta = aircraft.WindDirection-task.Bearing;

  GLIDE_RESULT result(task,VOpt);
  result.AltitudeDifference = -dh;
  result.EffectiveWindSpeed = W;
  result.EffectiveWindAngle = theta; 

  const double V = VOpt*cruise_efficiency;
  const double S = SinkRate(VOpt);

  // distance relation
  const double rho = S/mc;
  const double rhoplusone = 1.0+rho;
  //  const double k = (1+rho)*(1+rho);
  const double VonK = V/rhoplusone;
  const double costheta = cos(DEG_TO_RAD*(theta));

  const Quadratic q(-2.0*W*costheta,
                    W*W-VonK*VonK); // from (1)

  if (!q.check()) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    result.Distance = 0;
    return result;
  }
  const double Vn = q.solution_max();
  if (Vn<=0.0) {
    result.Solution = RESULT_MACCREADY_INSUFFICIENT;
    result.Distance = 0;
    return result;
  }

  double t_cl1 = 0.0;
  double distance = task.Distance;
  if (dh>0) {
    t_cl1 = dh/mc;
    const double wd = DEG_TO_RAD*(aircraft.WindDirection);
    const double tb = DEG_TO_RAD*(task.Bearing);
    const double dx= t_cl1*W*sin(wd)-task.Distance*sin(tb);
    const double dy= t_cl1*W*cos(wd)-task.Distance*cos(tb);
    distance = sqrt(dx*dx+dy*dy);
//    task.Bearing = RAD_TO_DEG*(atan2(dx,dy));
  }

  const double t_cr = distance/Vn;
  const double t_cl = t_cr*rho + (dh>0? t_cl1:0);

  result.TimeElapsed = t_cr+t_cl;
  result.HeightClimb = t_cl*mc;
  result.HeightGlide = t_cr*S-result.HeightClimb;
  result.AltitudeDifference = -dh+result.HeightClimb-result.HeightGlide;
  result.EffectiveWindSpeed = W*rhoplusone;

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

double 
GLIDE_RESULT::calc_vspeed(const double mc) 
{
  if ((mc>0.0) && (HeightGlide>0.0)) {
    // equivalent time to gain the height that was used
    TimeVirtual = HeightGlide/mc;
  } else {
    TimeVirtual = 0.0;
  }
  if (Distance>0.0) {
    return (TimeElapsed+TimeVirtual)/Distance;
  } else {
    return 0.0;
  }
}

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
    mac(_mac),
    mc(_mac.get_mc())
    {
    };
  virtual double f(const double V) {
    res = mac.solve_glide(aircraft, task, V);
    return res.calc_vspeed(mc);
  }
  virtual GLIDE_RESULT result() {
    find_min(20.0);
    return res;
  }
  GLIDE_RESULT res;
protected:
  const double mc;
  const AIRCRAFT_STATE &aircraft;
  const GLIDE_STATE &task;
  const MacCready &mac;
};

class MacCreadyVoptBasic: 
  public MacCreadyVopt
{
public:
  MacCreadyVoptBasic(const AIRCRAFT_STATE &_aircraft,
                const GLIDE_STATE &_task,
                const MacCready &_mac):
    MacCreadyVopt(_aircraft,_task,_mac) {};

  virtual double f(const double V) {
    res = mac.solve_glide_zerowind(aircraft, task, V);
    return res.calc_vspeed(mc);
  }
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
  MacCreadyVoptBasic mcvopt(aircraft, task, *this);
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
