/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "GlideState.hpp"
#include "GlidePolar.hpp"
#include "GlideResult.hpp"
#include "Navigation/Aircraft.hpp"
#include "Util/ZeroFinder.hpp"
#include "Util/Tolerances.hpp"

#define fixed_1mil fixed_int_constant(1000000)

#ifdef INSTRUMENT_TASK
// global, used for test harness
long count_mc = 0;
#endif

MacCready::MacCready(const GlidePolar &_glide_polar,
                     const fixed _cruise_efficiency)
  :glide_polar(_glide_polar), cruise_efficiency(_cruise_efficiency) {}

GlideResult 
MacCready::solve_vertical(const GlideState &task) const
{
  GlideResult result(task, glide_polar.GetVBestLD());

  // distance relation
  //   V*t_cr = W*(t_cl+t_cr)
  //     t_cr*(V-W)=W*t_cl
  //     t_cr = (W*t_cl)/(V-W)     .... (1)

  // height relation
  //   t_cl = (-dh+t_cr*S)/mc
  //     t_cl*mc = (-dh+(W*t_cl)/(V-W))    substitute (1)
  //     t_cl*mc*(V-W)= -dh*(V-W)+W*t_cl
  //     t_cl*(mc*(V-W)-W) = -dh*(V-W) .... (2)

  if (positive(task.altitude_difference)) {
    // immediate solution
    result.validity = GlideResult::RESULT_OK;
    return result;
  }
  
  const fixed V = glide_polar.GetVBestLD() * cruise_efficiency;
  const fixed denom1 = V - task.wind.norm;

  if (!positive(denom1)) {
    result.validity = GlideResult::RESULT_WIND_EXCESSIVE;
    return result;
  }
  const fixed denom2 = glide_polar.GetMC() * denom1 - task.wind.norm;
  if (!positive(denom2)) {
    result.validity = GlideResult::RESULT_MACCREADY_INSUFFICIENT;
    return result;
  }

  const fixed t_cl = -task.altitude_difference * denom1 / denom2; // from (2)
  const fixed t_cr = task.wind.norm * t_cl / denom1; // from (1)

  result.time_elapsed = t_cr + t_cl;
  result.height_climb = -task.altitude_difference;
  result.height_glide = fixed_zero;
  result.validity = GlideResult::RESULT_OK;

  return result;
}

GlideResult
MacCready::solve(const GlidePolar &glide_polar, const GlideState &task)
{
#ifdef INSTRUMENT_TASK
  count_mc++;
#endif
  MacCready mac(glide_polar, glide_polar.GetCruiseEfficiency());
  return mac.solve(task);
}

GlideResult
MacCready::solve_sink(const GlidePolar &glide_polar, const GlideState &task,
                      const fixed S)
{
#ifdef INSTRUMENT_TASK
  count_mc++;
#endif
  MacCready mac(glide_polar, glide_polar.GetCruiseEfficiency());
  return mac.solve_sink(task, S);
}

GlideResult
MacCready::solve_cruise(const GlideState &task) const
{
  const fixed VOpt = glide_polar.GetVBestLD();
  GlideResult result(task, VOpt);

  const fixed S = glide_polar.GetSBestLD();
  const fixed mc = glide_polar.GetMC();
  const fixed inv_mc = glide_polar.GetInvMC();
  const fixed rho = S * inv_mc;
  const fixed rhoplusone = fixed_one + rho;
  const fixed invrhoplusone = fixed_one / rhoplusone;

  const fixed Vn = task.CalcAverageSpeed(VOpt * cruise_efficiency * invrhoplusone);
  if (!positive(Vn)) {
    result.validity = GlideResult::RESULT_WIND_EXCESSIVE;
    result.vector.Distance = fixed_zero;
    return result;
  }

  fixed t_cl1 = fixed_zero;
  fixed distance = task.vector.Distance;
  if (negative(task.altitude_difference)) {
    t_cl1 = -task.altitude_difference * inv_mc;
    distance = task.DriftedDistance(t_cl1);
  }

  const fixed t = distance / Vn;
  const fixed t_cr = t * invrhoplusone;
  const fixed t_cl = t_cr * rho +
      (negative(task.altitude_difference) ? t_cl1 : fixed_zero);

  const fixed sink_glide = t_cr * S;

  result.time_elapsed = t;
  result.height_climb = t_cl * mc;
  result.height_glide = sink_glide - result.height_climb;
  result.altitude_difference -= sink_glide;
  result.effective_wind_speed *= rhoplusone;

  result.validity = GlideResult::RESULT_OK;

  return result;
}

GlideResult
MacCready::solve_glide(const GlideState &task, const fixed Vset, const fixed S,
                       const bool allow_partial) const
{
  // spend a lot of time in this function, so it should be quick!

  GlideResult result(task, Vset);

  // distance relation
  //   V*V=Vn*Vn+W*W-2*Vn*W*cos(theta)
  //     Vn*Vn-2*Vn*W*cos(theta)+W*W-V*V=0  ... (1)

  const fixed Vn = task.CalcAverageSpeed(Vset * cruise_efficiency);
  if (!positive(Vn)) {
    result.validity = GlideResult::RESULT_WIND_EXCESSIVE;
    result.vector.Distance = fixed_zero;

    return result;
  }

  result.validity = GlideResult::RESULT_OK;

  if (allow_partial) {
    const fixed Vndh = Vn * task.altitude_difference;

    // S/Vn > dh/task.Distance
    if (S * task.vector.Distance > Vndh) {
      result.validity = GlideResult::RESULT_PARTIAL;
      if (negative(task.altitude_difference))
        // insufficient height, and can't climb
        result.vector.Distance = fixed_zero;
      else
        // frac*task.Distance;
        result.vector.Distance = Vndh / S;
    }
  }

  const fixed t_cr = result.vector.Distance / Vn;
  result.time_elapsed = t_cr;
  result.height_glide = t_cr * S;
  result.altitude_difference -= result.height_glide;
  result.distance_to_final = fixed_zero;

  return result;
}

GlideResult
MacCready::solve_glide(const GlideState &task, const fixed Vset,
                       const bool allow_partial) const
{
  const fixed S = glide_polar.SinkRate(Vset);
  return solve_glide(task, Vset, S, allow_partial);
}

GlideResult
MacCready::solve_sink(const GlideState &task, const fixed S) const
{
  const fixed h_offset = fixed_1mil;
  GlideState virt_task = task;
  virt_task.altitude_difference += h_offset;
  GlideResult res = solve_glide(task, glide_polar.GetVBestLD(), S);
  res.altitude_difference -= h_offset;
  return res;
}

GlideResult
MacCready::solve(const GlideState &task) const
{
  if (!positive(task.vector.Distance))
    return solve_vertical(task);

  if (!positive(glide_polar.GetMC()))
    // whole task must be glide
    return optimise_glide(task, false);

  if (!positive(task.altitude_difference))
    // whole task climb-cruise
    return solve_cruise(task);

  // task partial climb-cruise, partial glide

  // calc first final glide part
  GlideResult result_fg = optimise_glide(task, true);
  if (result_fg.validity == GlideResult::RESULT_OK)
    // whole task final glided
    return result_fg;

  // climb-cruise remainder of way

  GlideState sub_task = task;
  sub_task.vector.Distance -= result_fg.vector.Distance;
  sub_task.min_height += result_fg.height_glide;
  sub_task.altitude_difference -= result_fg.height_glide;

  GlideResult result_cc = solve_cruise(sub_task);
  result_cc.Add(result_fg);

  return result_cc;
}

/**
 * Class used to find VOpt for a MacCready setting, for final glide
 * calculations.  Intended to be used temporarily only.
 */
class MacCreadyVopt: public ZeroFinder
{
public:
  /**
   * Constructor
   *
   * @param _task Task to solve for
   * @param _mac MacCready object to use for search
   * @param vmin Min speed for search range
   * @param vmax Max speed for search range
   * @param _allow_partial Whether to allow partial solutions or not
   *
   * @return Initialised object (not yet searched)
   */
  MacCreadyVopt(const GlideState &_task, const MacCready &_mac,
                const fixed _inv_mc, const fixed & vmin, const fixed & vmax,
                const bool _allow_partial) :
    ZeroFinder(vmin, vmax, fixed(TOLERANCE_MC_OPT_GLIDE)),
    task(_task),
    mac(_mac),
    inv_mc(_inv_mc),
    allow_partial(_allow_partial)
    {
    }

  /**
   * Function to optimise in search
   *
   * \note the f(x) is magnified because with fixed, find_min can
   *   fail with too small df/dx
   *
   * @param V cruise true air speed (m/s)
   * @return Virtual speed (m/s) of flight
   */
  fixed
  f(const fixed V)
  {
    res = mac.solve_glide(task, V, allow_partial);
    return res.CalcVSpeed(inv_mc) * fixed_360;
  }
  
  /**
   * Perform search for best cruise speed and return result 
   * @return Glide solution (optimum)
   */
  GlideResult
  result(const fixed &vinit)
  {
    find_min(vinit);
    return res;
  }

private:
  GlideResult res;
  const GlideState &task;
  const MacCready &mac;
  const fixed inv_mc;
  const bool allow_partial;
};

GlideResult
MacCready::optimise_glide(const GlideState &task, const bool allow_partial) const
{
  MacCreadyVopt mcvopt(task, *this, glide_polar.GetInvMC(),
                       glide_polar.GetVMin(), glide_polar.GetVMax(),
                       allow_partial);

  return mcvopt.result(glide_polar.GetVMin());
}

/*
  // distance relation

  (W*(1+rho))**2+((1+rho)*Vn)**2-2*W*(1+rho)*Vn*(1+rho)*costheta-V*V

subs rho=(gamma*Vn+S)/mc
-> rho = S/mc
   k = 1+rho = (S+mc)/mc
    rho = (S+M)/M-1

*/
