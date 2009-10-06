#include "MacCready.hpp"
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "Util/Quadratic.hpp"

bool GLIDE_RESULT::superior(const GLIDE_RESULT &s2) const {
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
};


double MacCready::SinkRate(double V) const
{
  double dV = (V-20.0)/20.0;
  return 1.0+dV*dV*4.0;
}

GLIDE_RESULT MacCready::solve_vertical(const AIRCRAFT_STATE &aircraft,
                                       const GLIDE_STATE &task,
				       const double V)
{
  double S = SinkRate(V);
  double M = task.MacCready;
  double W = aircraft.WindSpeed;

  GLIDE_RESULT result;
  result.TrackBearing = task.Bearing;
  result.CruiseTrackBearing = -aircraft.WindDirection;
  result.VOpt = V;
  result.Distance = task.Distance;
  result.HeightClimb = 0;
  result.HeightGlide = 0;
  result.TimeElapsed = 0;
  result.TimeVirtual = 0;

  double dh = task.MinHeight-aircraft.Altitude;

  // distance relation
  //   V*t_cr = W*(t_cl+t_cr)
  //     t_cr*(V-W)=W*t_cl
  //     t_cr = (W*t_cl)/(V-W)     .... (1)

  // height relation
  //   t_cl = (dh+t_cr*S)/M
  //     t_cl*M = (dh+(W*t_cl)/(V-W))    substitute (1)
  //     t_cl*M*(V-W)= dh*(V-W)+W*t_cl
  //     t_cl*(M*(V-W)-W) = dh*(V-W) .... (2)

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

  double denom2 = M*(V-W)-W;
  if (denom2<=0) {
    result.Solution = RESULT_MACCREADY_INSUFFICIENT;
    return result;
  } 
    
  double t_cl = dh*(V-W)/denom2; // from (2)
  double t_cr = (W*t_cl)/denom1; // from (1)

  result.TimeElapsed = t_cr+t_cl;
  result.TimeVirtual = 0;
  result.HeightClimb = dh+t_cr*S;
  result.HeightGlide = 0;
  result.Solution = RESULT_OK;

  return result;
}


GLIDE_RESULT MacCready::solve_glide(const AIRCRAFT_STATE &aircraft,
                                    const GLIDE_STATE &task,
				    const double V)
{
  double S = SinkRate(V);
  double M = task.MacCready;
  double W = aircraft.WindSpeed;
  double theta = aircraft.WindDirection;

  GLIDE_RESULT result;
  result.TrackBearing = task.Bearing;
  result.CruiseTrackBearing = task.Bearing;
  result.VOpt = V;
  result.Distance = 0;
  result.HeightClimb = 0;
  result.HeightGlide = 0;
  result.TimeElapsed = 0;
  result.TimeVirtual = 0;

  // distance relation
  //   V*V=Vn*Vn+W*W-2*Vn*W*cos(theta)
  //     Vn*Vn-2*Vn*W*cos(theta)+W*W-V*V=0  ... (1)

  Quadratic q(1.0,2.0*W*cos(theta),W*W-V*V); // from (1)

  if (!q.check()) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }
  double Vn = q.solution_max();
  if (Vn<=0.0) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }

  double dh = task.MinHeight-aircraft.Altitude;
  double gamma = dh/task.Distance;

  if (gamma*Vn+S>0) {
    if (gamma>0) {
      // insufficient height, and can't climb
      result.Distance = 0;
      result.Solution = RESULT_MACCREADY_INSUFFICIENT;
      return result;
    } else {
      // frac = gamma/(-S/Vn)
      double frac = gamma/(-S/Vn);
      result.Distance = frac*task.Distance;
      result.Solution = RESULT_PARTIAL;
    }
  } else {
    result.Distance = task.Distance;
    result.Solution = RESULT_OK;
  }

  double t_cr = result.Distance/Vn;
  result.TimeElapsed = t_cr;
  result.HeightClimb = 0;
  result.HeightGlide = S*t_cr;

  if (M>0) {
    // equivalent time to gain the height that was used
    result.TimeVirtual = result.HeightGlide/M;
  }

  return result;
}


GLIDE_RESULT MacCready::solve_cruise(const AIRCRAFT_STATE &aircraft,
                                     const GLIDE_STATE &task,
                                     const double V)
{
  double S = SinkRate(V);
  double M = task.MacCready;
  double W = aircraft.WindSpeed;
  double theta = aircraft.WindDirection;

  GLIDE_RESULT result;
  result.TrackBearing = task.Bearing;
  result.CruiseTrackBearing = task.Bearing;
  result.VOpt = V;
  result.HeightClimb = 0;
  result.HeightGlide = 0;
  result.TimeElapsed = 0;
  result.TimeVirtual = 0;
  result.Distance = 0;

  double dh = task.MinHeight-aircraft.Altitude;
  double gamma = dh/task.Distance;

  double k = W*gamma/M;
  double L = 1+S/M;
  double WL = W*L;
  double costheta = cos(theta);

  // distance relation

  Quadratic q(1.0-2.0*k*costheta-k*k,
              2.0*WL*(k-costheta),
              WL*WL-V*V); // from (1)

  if (!q.check()) {
    result.Solution = RESULT_WIND_EXCESSIVE;
    return result;
  }
  double Vn = q.solution_max();
  if (Vn<=0.0) {
    result.Solution = RESULT_MACCREADY_INSUFFICIENT;
    return result;
  }

  double rho = (gamma*Vn+S)/M;

  double t_cr = task.Distance/Vn;
  double t_cl = std::max(0.0,t_cr*rho);
    
  result.TimeElapsed = t_cr+t_cl;
  result.HeightClimb = t_cl*M;
  result.HeightGlide = t_cr*S-result.HeightClimb;
  result.Distance = task.Distance;

  // equivalent time to gain the height that was used
  result.TimeVirtual = std::max(0.0,result.HeightGlide/M);

  result.Solution = RESULT_OK;

  return result;
}


GLIDE_RESULT MacCready::solve(const AIRCRAFT_STATE &aircraft,
                              const GLIDE_STATE &task)
{
  if (task.Distance==0) {
    return optimise(aircraft, task, &MacCready::solve_vertical);
  } 
  if (task.MacCready==0) {
    return optimise(aircraft, task, &MacCready::solve_glide);
  }

  // check first if can final glide
  GLIDE_RESULT result = optimise(aircraft, task, &MacCready::solve_glide);
  if (result.Solution == RESULT_OK) {
    return result;
  }

  GLIDE_STATE sub_task = task;
  sub_task.Distance -= result.Distance;
  sub_task.MinHeight += result.HeightGlide;

  GLIDE_RESULT sub_result = optimise(aircraft, sub_task, &MacCready::solve_cruise);

  sub_result.TimeElapsed += result.TimeElapsed;
  sub_result.HeightGlide += result.HeightGlide-sub_result.HeightClimb;
  sub_result.HeightClimb += result.HeightClimb;
  sub_result.Distance += result.Distance;

  // equivalent time to gain the height that was used
  sub_result.TimeVirtual = std::max(0.0,sub_result.HeightGlide/task.MacCready)
    +result.TimeVirtual;

  sub_result.Solution = sub_result.Solution;

  return sub_result;
}


GLIDE_RESULT MacCready::optimise(const AIRCRAFT_STATE &aircraft,
				 const GLIDE_STATE &task,
				 Solver_t solver)
{
  GLIDE_RESULT best_result;
  best_result.Solution = RESULT_NOSOLUTION;

  for (double V=1.0; V<=50.0; V+= 0.5) {
    GLIDE_RESULT this_result = ((this)->*solver)(aircraft, task, V);
    if (this_result.superior(best_result)) {
      best_result = this_result;
    }
  }
  return best_result;
}
