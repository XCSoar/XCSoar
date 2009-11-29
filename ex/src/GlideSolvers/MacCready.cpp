/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */
#include "MacCready.hpp"
#include <assert.h>
#include <algorithm>
#include "Math/Geometry.hpp"
#include "GlideState.hpp"
#include "GlidePolar.hpp"
#include "GlideResult.hpp"
#include "Math/NavFunctions.hpp"
#include "Navigation/Aircraft.hpp"


MacCready::MacCready(const GlidePolar &_glide_polar,
                     const double _cruise_efficiency):
  glide_polar(_glide_polar),
  cruise_efficiency(_cruise_efficiency)
{
  
}


GlideResult 
MacCready::solve_vertical(const GlideState &task) const
{

//  double S = SinkRate(VOpt);

  GlideResult result(task,glide_polar.get_VbestLD());

  // distance relation
  //   V*t_cr = W*(t_cl+t_cr)
  //     t_cr*(V-W)=W*t_cl
  //     t_cr = (W*t_cl)/(V-W)     .... (1)

  // height relation
  //   t_cl = (-dh+t_cr*S)/mc
  //     t_cl*mc = (-dh+(W*t_cl)/(V-W))    substitute (1)
  //     t_cl*mc*(V-W)= -dh*(V-W)+W*t_cl
  //     t_cl*(mc*(V-W)-W) = -dh*(V-W) .... (2)

  if (task.AltitudeDifference>0) {
    // immediate solution
    result.Solution = GlideResult::RESULT_OK;
    return result;
  }
  
  const double V = glide_polar.get_VbestLD()*cruise_efficiency;
  const double denom1 = V-task.EffectiveWindSpeed;
  if (denom1<=0) {
    result.Solution = GlideResult::RESULT_WIND_EXCESSIVE;
    return result;
  }
  const double denom2 = glide_polar.get_mc()*denom1-task.EffectiveWindSpeed;
  if (denom2<=0) {
    result.Solution = GlideResult::RESULT_MACCREADY_INSUFFICIENT;
    return result;
  } 

  const double t_cl = -task.AltitudeDifference*denom1/denom2; // from (2)
  const double t_cr = (task.EffectiveWindSpeed*t_cl)/denom1; // from (1)

  result.TimeElapsed = t_cr+t_cl;
  result.HeightClimb = -task.AltitudeDifference;
  result.HeightGlide = 0;
  result.Solution = GlideResult::RESULT_OK;

  return result;
}


GlideResult 
MacCready::solve_cruise(const GlideState &task) const
{
  const double VOpt = glide_polar.get_VbestLD();
  GlideResult result(task,VOpt);

  const double S = glide_polar.get_SbestLD();
  const double mc = glide_polar.get_mc();
  const double rho = S/mc;
  const double rhoplusone = 1.0+rho;
  const double invrhoplusone = 1.0/rhoplusone;
  const double Vn = task.calc_ave_speed(VOpt*cruise_efficiency*invrhoplusone);
  if (Vn<=0.0) {
    result.Solution = GlideResult::RESULT_WIND_EXCESSIVE;
    result.Vector.Distance = 0;
    return result;
  }

  double t_cl1 = 0.0;
  double distance = task.Vector.Distance;
  if (task.AltitudeDifference<0) {
    t_cl1 = -task.AltitudeDifference/mc;
    distance = task.drifted_distance(t_cl1);
  }

  const double t = distance/Vn;
  const double t_cr = t*invrhoplusone;
  const double t_cl = t_cr*rho + (task.AltitudeDifference<0? t_cl1:0);

  result.TimeElapsed = t;
  result.HeightClimb = t_cl*mc;
  result.HeightGlide = t_cr*S-result.HeightClimb;
  result.AltitudeDifference -= result.HeightClimb+result.HeightGlide;
  result.EffectiveWindSpeed *= rhoplusone;

  result.Solution = GlideResult::RESULT_OK;

  return result;
}


GlideResult 
MacCready::solve_glide(const GlideState &task,
                       const double Vset,
                       const double S) const
{
  // spend a lot of time in this function, so it should be quick!

  GlideResult result(task,Vset);

  // distance relation
  //   V*V=Vn*Vn+W*W-2*Vn*W*cos(theta)
  //     Vn*Vn-2*Vn*W*cos(theta)+W*W-V*V=0  ... (1)

  const double Vn = task.calc_ave_speed(Vset*cruise_efficiency);
  if (Vn<=0.0) {
    result.Solution = GlideResult::RESULT_WIND_EXCESSIVE;
    result.Vector.Distance = 0;
    return result;
  }

  const double Vndh = Vn*task.AltitudeDifference;

  if (S*task.Vector.Distance>Vndh) { // S/Vn > dh/task.Distance
    if (task.AltitudeDifference<0) { 
      // insufficient height, and can't climb
      result.Vector.Distance = 0;
      result.Solution = GlideResult::RESULT_MACCREADY_INSUFFICIENT;
      return result;
    } else {
      result.Vector.Distance = Vndh/S; // frac*task.Distance; 
      result.Solution = GlideResult::RESULT_PARTIAL;
    }
  } else {
    result.Solution = GlideResult::RESULT_OK;
  }
  const double t_cr = result.Vector.Distance/Vn;
  result.TimeElapsed = t_cr;
  result.HeightGlide = t_cr*S;
  result.AltitudeDifference -= result.HeightGlide;
  result.DistanceToFinal = 0;

  return result;
}


GlideResult 
MacCready::solve_glide(const GlideState &task,
                       const double Vset) const
{
  const double S = glide_polar.SinkRate(Vset);
  return solve_glide(task, Vset, S);
}


GlideResult 
MacCready::solve_sink(const GlideState &task,
                      const double S) const
{
  const double h_offset = 1.0e6;
  GlideState virt_task = task;
  virt_task.AltitudeDifference += h_offset;
  GlideResult res = solve_glide(task, glide_polar.get_VbestLD(), S);
  res.AltitudeDifference -= h_offset;
  return res;
}



GlideResult 
MacCready::solve(const GlideState &task) const
{
  if (task.Vector.Distance==0) {
    return solve_vertical(task);
  } else if (glide_polar.get_mc()==0) {
    // whole task must be glide
    return optimise_glide(task);
  } else if (task.AltitudeDifference<=0) {
    // whole task climb-cruise
    return solve_cruise(task);
  } else {
    // task partial climb-cruise, partial glide

    // calc first final glide part
    GlideResult result_fg = optimise_glide(task);
    if (result_fg.Solution == GlideResult::RESULT_OK) {
      // whole task final glided
      return result_fg;
    }
    
    // climb-cruise remainder of way
    
    GlideState sub_task = task;
    sub_task.Vector.Distance -= result_fg.Vector.Distance;
    sub_task.MinHeight += result_fg.HeightGlide;
    sub_task.AltitudeDifference -= result_fg.HeightGlide;
    
    GlideResult result_cc = solve_cruise(sub_task);
    result_cc.add(result_fg);
    
    return result_cc;
  }
}

#include "Util/ZeroFinder.hpp"
#include "Util/Tolerances.hpp"

/**
 * Class used to find VOpt for a MacCready setting, for final glide
 * calculations.  Intended to be used temporarily only.
 */
class MacCreadyVopt: 
  public ZeroFinder
{
public:
/** 
 * Constructor
 * 
 * @param _task Task to solve for
 * @param _mac MacCready object to use for search
 * 
 * @return Initialised object (not yet searched)
 */
  MacCreadyVopt(const GlideState &_task,
                const MacCready &_mac):
    ZeroFinder(15.0,75.0, TOLERANCE_MC_OPT_GLIDE),
    task(_task),
    mac(_mac),
    mc(_mac.get_mc())
    {
    };

  /**
   * Function to optimise in search
   *
   * @param V cruise true air speed (m/s)
   * @return Virtual speed (m/s) of flight
   */
  double f(const double V) {
    res = mac.solve_glide(task, V);
    return res.calc_vspeed(mc);
  }
  
  /**
   * Perform search for best cruise speed and return result 
   * @return Glide solution (optimum)
   */
  GlideResult result() {
    find_min(20.0);
    return res;
  }
private:
  GlideResult res;
  const GlideState &task;
  const MacCready &mac;
  const double mc;
};


GlideResult 
MacCready::optimise_glide(const GlideState &task) const
{
  MacCreadyVopt mcvopt(task, *this);
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

double MacCready::get_mc() const {
  return glide_polar.get_mc();
}
