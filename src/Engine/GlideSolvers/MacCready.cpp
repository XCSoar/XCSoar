/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "GlideState.hpp"
#include "GlidePolar.hpp"
#include "GlideResult.hpp"
#include "Math/ZeroFinder.hpp"
#include "Util/Tolerances.hpp"

#include <assert.h>

MacCready::MacCready(const GlideSettings &_settings,
                     const GlidePolar &_glide_polar,
                     const double _cruise_efficiency)
  :settings(_settings), glide_polar(_glide_polar),
   cruise_efficiency(_cruise_efficiency) {}

MacCready::MacCready(const GlideSettings &_settings,
                     const GlidePolar &_glide_polar)
  :settings(_settings), glide_polar(_glide_polar),
   cruise_efficiency(_glide_polar.GetCruiseEfficiency()) {}

GlideResult 
MacCready::SolveVertical(const GlideState &task) const
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

  if (task.altitude_difference >= 0) {
    // immediate solution
    result.pure_glide_height = 0;
    result.height_climb = 0;
    result.height_glide = 0;
    result.time_elapsed = 0;
    result.validity = GlideResult::Validity::OK;
    return result;
  }
  
  const auto v = glide_polar.GetVBestLD() * cruise_efficiency;
  const auto denom1 = v - task.wind.norm;

  if (denom1 <= 0) {
    result.validity = GlideResult::Validity::WIND_EXCESSIVE;
    return result;
  }
  const auto denom2 = glide_polar.GetMC() * denom1 - task.wind.norm;
  if (denom2 <= 0) {
    result.validity = GlideResult::Validity::MACCREADY_INSUFFICIENT;
    return result;
  }

  // from (2)
  const auto time_climb = -task.altitude_difference * denom1 / denom2;
  // from (1)
  const auto time_cruise = task.wind.norm * time_climb / denom1; 

  result.pure_glide_height = 0;
  result.time_elapsed = time_cruise + time_climb;
  result.time_virtual = 0;
  result.height_climb = -task.altitude_difference;
  result.height_glide = 0;
  result.validity = GlideResult::Validity::OK;

  return result;
}

GlideResult
MacCready::Solve(const GlideSettings &settings, const GlidePolar &glide_polar,
                 const GlideState &task)
{
  const MacCready mac(settings, glide_polar);
  return mac.Solve(task);
}

GlideResult
MacCready::SolveSink(const GlideSettings &settings,
                     const GlidePolar &glide_polar, const GlideState &task,
                     const double sink_rate)
{
  const MacCready mac(settings, glide_polar);
  return mac.SolveSink(task, sink_rate);
}

GlideResult
MacCready::SolveCruise(const GlideState &task) const
{
  // cruise speed for current MC (m/s)
  const auto mc_speed = glide_polar.GetVBestLD();

  GlideResult result(task, mc_speed);

  // sink rate at current MC speed (m/s)
  const auto mc_sink_rate = glide_polar.GetSBestLD();

  // MC value (m/s)
  const auto mc = glide_polar.GetMC();

  // Inverse MC value (s/m)
  const auto inv_mc = glide_polar.GetInvMC();

  /*
      |      rho = S / MC
      *..                       cruise speed
   MC |  ´--..                 /
    --+-------´´*-..-----------+------------
      |        /    ´´--..     | S
      |       /   ..---...´´-..|
      |      /  .´        ´´´--*.
      |     /                    ´´-.
      |    /                         ´.
      |   /
      |  resulting speed
  */

  // Sink rate divided by MC value
  // same as (cruise speed - resulting speed) / resulting speed
  const auto rho = mc_sink_rate * inv_mc;

  // quotient of cruise speed over resulting speed (> 1.0)
  const auto rho_plus_one = 1 + rho;

  // quotient of resulting speed over cruise speed (0 .. 1)
  const auto inv_rho_plus_one = 1. / rho_plus_one;

  const auto estimated_speed =
      task.CalcAverageSpeed(mc_speed * cruise_efficiency * inv_rho_plus_one);
  if (estimated_speed <= 0) {
    result.validity = GlideResult::Validity::WIND_EXCESSIVE;
    result.vector.distance = 0;
    return result;
  }

  double time_climb_drift = 0;
  auto distance_with_climb_drift = task.vector.distance;

  // Calculate additional distance_with_climb_drift/time due to wind drift while circling
  if (task.altitude_difference < 0) {
    time_climb_drift = -task.altitude_difference * inv_mc;
    distance_with_climb_drift = task.DriftedDistance(time_climb_drift);
  }

  // Estimated time to finish the task
  const auto estimated_time = distance_with_climb_drift / estimated_speed;
  // Estimated time in cruise
  const auto time_cruise = estimated_time * inv_rho_plus_one;
  // Estimated time in climb (including wind drift while circling)
  const auto time_climb = time_cruise * rho + time_climb_drift;

  const auto sink_glide = time_cruise * mc_sink_rate;

  result.time_elapsed = estimated_time + time_climb_drift;
  result.time_virtual = 0;
  result.height_climb = time_climb * mc;
  result.height_glide = sink_glide;
  result.altitude_difference -= sink_glide;
  result.effective_wind_speed *= rho_plus_one;

  result.validity = GlideResult::Validity::OK;
  result.pure_glide_height = task.vector.distance /
    glide_polar.GetLDOverGround(task.vector.bearing, task.wind);
  result.pure_glide_altitude_difference -= result.pure_glide_height;

  return result;
}

GlideResult
MacCready::SolveGlide(const GlideState &task, const double v_set,
                      const double sink_rate, const bool allow_partial) const
{
  // spend a lot of time in this function, so it should be quick!

  GlideResult result(task, v_set);

  // distance relation
  //   V*V=Vn*Vn+W*W-2*Vn*W*cos(theta)
  //     Vn*Vn-2*Vn*W*cos(theta)+W*W-V*V=0  ... (1)

  const auto estimated_speed = task.CalcAverageSpeed(v_set * cruise_efficiency);
  if (estimated_speed <= 0) {
    result.validity = GlideResult::Validity::WIND_EXCESSIVE;
    result.vector.distance = 0;
    return result;
  }

  result.validity = GlideResult::Validity::OK;

  if (allow_partial) {
    const auto Vndh = estimated_speed * task.altitude_difference;

    // S/Vn > dh/task.Distance
    if (sink_rate * task.vector.distance > Vndh) {
      if (task.altitude_difference < 0)
        // insufficient height, and can't climb
        result.vector.distance = 0;
      else
        // frac*task.Distance;
        result.vector.distance = Vndh / sink_rate;
    }
  }

  const auto time_cruise = result.vector.distance / estimated_speed;
  result.time_elapsed = time_cruise;
  result.height_climb = 0;
  result.height_glide = time_cruise * sink_rate;
  result.pure_glide_height = result.height_glide;
  result.altitude_difference -= result.height_glide;
  result.pure_glide_altitude_difference -= result.pure_glide_height;

  // equivalent time to gain the height that was used
  result.time_virtual = result.height_glide * glide_polar.GetInvMC();

  return result;
}

GlideResult
MacCready::SolveGlide(const GlideState &task, const double v_set,
                       const bool allow_partial) const
{
  const auto sink_rate = glide_polar.SinkRate(v_set);
  return SolveGlide(task, v_set, sink_rate, allow_partial);
}

GlideResult
MacCready::SolveSink(const GlideState &task, const double sink_rate) const
{
  return SolveGlide(task, glide_polar.GetVBestLD(), sink_rate);
}

GlideResult
MacCready::SolveStraight(const GlideState &task) const
{
  if (!glide_polar.IsValid()) {
    /* can't solve without a valid GlidePolar() */
    GlideResult result;
    result.Reset();
    return result;
  }

  if (task.vector.distance <= 0)
    return SolveVertical(task);

  if (glide_polar.GetMC() <= 0)
    // whole task must be glide
    return OptimiseGlide(task);

  return SolveGlide(task, glide_polar.GetVBestLD());
}

GlideResult
MacCready::Solve(const GlideState &task) const
{
  if (!glide_polar.IsValid()) {
    /* can't solve without a valid GlidePolar() */
    GlideResult result;
    result.Reset();
    return result;
  }

  if (task.vector.distance <= 0)
    return SolveVertical(task);

  if (glide_polar.GetMC() <= 0)
    // whole task must be glide
    return OptimiseGlide(task, false);

  if (task.altitude_difference < 0)
    // whole task climb-cruise
    return SolveCruise(task);

  // task partial climb-cruise, partial glide

  // calc first final glide part
  GlideResult result_fg = SolveGlide(task, glide_polar.GetVBestLD(), true);
  if (result_fg.validity == GlideResult::Validity::OK &&
      task.vector.distance - result_fg.vector.distance <= 0)
    // whole task final glided
    return result_fg;

  // climb-cruise remainder of way

  GlideState sub_task = task;
  sub_task.vector.distance -= result_fg.vector.distance;
  sub_task.altitude_difference -= result_fg.height_glide;

  GlideResult result_cc = SolveCruise(sub_task);
  result_fg.Add(result_cc);

  return result_fg;
}

/**
 * Class used to find VOpt to optimize glide distance, for final glide
 * calculations.  Intended to be used temporarily only.
 */
class MacCreadyVopt: public ZeroFinder
{
  GlideResult res;
  const GlideState &task;
  const MacCready &mac;
  const bool allow_partial;

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
                double vmin, double vmax,
                const bool _allow_partial)
    :ZeroFinder(vmin, vmax, TOLERANCE_MC_OPT_GLIDE),
     task(_task),
     mac(_mac),
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
   * @return Inverse LD
   */
  double f(const double v) {
    res = mac.SolveGlide(task, v, allow_partial);
    if (!res.IsOk() || res.vector.distance <= 0)
      /* the solver failed: return a large value that will be
         discarded by ZeroFinder */
      return 1000000;

    return res.height_glide * 1024 / res.vector.distance;
  }
  
  /**
   * Perform search for best cruise speed and return Result
   * @return Glide solution (optimum)
   */
  GlideResult Result(const double v_init) {
    auto v = find_min(v_init);
    return mac.SolveGlide(task, v, allow_partial);
  }
};

GlideResult
MacCready::OptimiseGlide(const GlideState &task, const bool allow_partial) const
{
  assert(glide_polar.GetMC() <= 0);

  MacCreadyVopt mc_vopt(task, *this,
                       glide_polar.GetVMin(), glide_polar.GetVMax(),
                       allow_partial);

  return mc_vopt.Result(glide_polar.GetVMin());
}

/*
  // distance relation

  (W*(1+rho))**2+((1+rho)*Vn)**2-2*W*(1+rho)*Vn*(1+rho)*costheta-V*V

subs rho=(gamma*Vn+S)/mc
-> rho = S/mc
   k = 1+rho = (S+mc)/mc
    rho = (S+M)/M-1

*/
