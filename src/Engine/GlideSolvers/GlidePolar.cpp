/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "GlidePolar.hpp"
#include "GlideState.hpp"
#include "GlideResult.hpp"
#include "MacCready.hpp"
#include "Math/ZeroFinder.hpp"
#include "Math/Quadratic.hpp"
#include "Util/Tolerances.hpp"
#include "Util/Clamp.hpp"
#include "Navigation/Aircraft.hpp"

#include <assert.h>

GlidePolar::GlidePolar(const fixed _mc, const fixed _bugs, const fixed _ballast) :
  mc(_mc),
  bugs(_bugs),
  ballast(_ballast),
  cruise_efficiency(fixed(1)),
  VbestLD(fixed(0)),
  Vmax(fixed(75.0)),
  Vmin(fixed(0)),
  ideal_polar(fixed(0.00157), fixed(-0.0734), fixed(1.48)),
  ballast_ratio(0.3),
  reference_mass(300),
  dry_mass(reference_mass),
  wing_area(fixed(0))
{
  Update();

  // Calculate inv_mc
  SetMC(mc);
}

void
GlidePolar::Update()
{
  assert(positive(bugs));

  if (!ideal_polar.IsValid()) {
    Vmin = Vmax = fixed(0);
    return;
  }

  const fixed loading_factor = sqrt(GetTotalMass() / reference_mass);
  const fixed inv_bugs = fixed(1)/bugs;

  polar.a = inv_bugs * ideal_polar.a / loading_factor;
  polar.b = inv_bugs * ideal_polar.b;
  polar.c = inv_bugs * ideal_polar.c * loading_factor;

  assert(polar.IsValid());

  UpdateSMax();
  UpdateSMin();
}

void
GlidePolar::UpdateSMax()
{
  assert(polar.IsValid());

  Smax = SinkRate(Vmax);
}

void
GlidePolar::SetBugs(const fixed clean)
{
  assert(positive(clean) && !positive(clean - fixed(1)));
  bugs = clean;
  Update();
}

void
GlidePolar::SetBallast(const fixed bal)
{
  assert(!negative(bal));
  SetBallastLitres(bal * ballast_ratio * reference_mass);
}

void
GlidePolar::SetBallastLitres(const fixed litres)
{
  assert(!negative(litres));
  ballast = litres;
  Update();
}

void
GlidePolar::SetMC(const fixed _mc)
{
  mc = _mc;

  if (positive(mc))
    inv_mc = fixed(1)/mc;
  else
    inv_mc = fixed(0);

  if (IsValid())
    UpdateBestLD();
}

fixed
GlidePolar::MSinkRate(const fixed V) const
{
  return SinkRate(V) + mc;
}

fixed
GlidePolar::SinkRate(const fixed V) const
{
  assert(polar.IsValid());

  return V * (V * polar.a + polar.b) + polar.c;
}

fixed
GlidePolar::SinkRate(const fixed V, const fixed n) const
{
  const fixed w0 = SinkRate(V);
  const fixed vl = VbestLD / max(Half(VbestLD), V);
  return max(fixed(0),
             w0 + (V / Double(bestLD)) * (sqr(n) - fixed(1)) * sqr(vl));
}

#if 0
/**
 * Finds VOpt for a given MacCready setting
 * Intended to be used temporarily.
 */
class GlidePolarVopt: public ZeroFinder
{
  const GlidePolar &polar;

public:
  /**
   * Constructor.
   *
   * @param _polar Glide polar to optimise
   * @param vmin Minimum speed to search (m/s)
   * @param vmax Maximum speed to search (m/s)
   *
   * @return Initialised object (no search yet)
   */
  GlidePolarVopt(const GlidePolar &_polar, const fixed vmin, const fixed vmax):
    ZeroFinder(vmin, vmax, fixed(TOLERANCE_POLAR_BESTLD)),
    polar(_polar)
  {
  }

  /**
   * Glide ratio function
   *
   * @param V Speed (m/s)
   *
   * @return MacCready-adjusted inverse glide ratio
   */
  fixed
  f(const fixed V)
  {
    return -V/polar.MSinkRate(V);
  }
};
#endif

void
GlidePolar::UpdateBestLD()
{
#if 0
  // this method to be used if polar is not parabolic
  GlidePolarVopt gpvopt(*this, Vmin, Vmax);
  VbestLD = gpvopt.find_min(VbestLD);
#else
  assert(polar.IsValid());
  assert(!negative(mc));

  VbestLD = Clamp(sqrt((polar.c + mc) / polar.a), Vmin, Vmax);
  SbestLD = SinkRate(VbestLD);
  bestLD = VbestLD / SbestLD;
#endif
}

#if 0
/**
 * Finds min sink speed.
 * Intended to be used temporarily.
 */
class GlidePolarMinSink: public ZeroFinder
{
  const GlidePolar &polar;

public:
  /**
   * Constructor.
   *
   * @param _polar Glide polar to optimise
   * @param vmax Maximum speed to search (m/s)
   *
   * @return Initialised object (no search yet)
   */
  GlidePolarMinSink(const GlidePolar &_polar, const fixed vmax):
    ZeroFinder(fixed(1), vmax, fixed(TOLERANCE_POLAR_MINSINK)),
    polar(_polar)
  {
  }

  fixed
  f(const fixed V)
  {
    return polar.SinkRate(V);
  }
};
#endif

void 
GlidePolar::UpdateSMin()
{
#if 0
  // this method to be used if polar is not parabolic
  GlidePolarMinSink gpminsink(*this, Vmax);
  Vmin = gpminsink.find_min(Vmin);
#else
  assert(polar.IsValid());

  Vmin = min(Vmax, -polar.b / Double(polar.a));
  Smin = SinkRate(Vmin);
#endif

  UpdateBestLD();
}

bool 
GlidePolar::IsGlidePossible(const GlideState &task) const
{
  if (!positive(task.altitude_difference))
    return false;

  // broad test assuming tailwind at best LD (best case)
  if ((VbestLD + task.wind.norm) * task.altitude_difference
      < task.vector.distance * SbestLD)
    return false;

  return true;
}

/**
 * Finds speed to fly for a given MacCready setting
 * Intended to be used temporarily.
 *
 * This finds the speed that maximises the glide angle over the ground
 */
class GlidePolarSpeedToFly: public ZeroFinder
{
  const GlidePolar &polar;
  const fixed m_net_sink_rate;
  const fixed m_head_wind;

public:
  /**
   * Constructor.
   *
   * @param _polar Glide polar to optimise
   * @param net_sink_rate Instantaneous netto sink rate (m/s), positive down
   * @param head_wind Head wind component (m/s)
   * @param vmin Minimum speed to search (m/s)
   * @param vmax Maximum speed to search (m/s)
   *
   * @return Initialised object (no search yet)
   */
  GlidePolarSpeedToFly(const GlidePolar &_polar, const fixed net_sink_rate,
                       const fixed head_wind, const fixed vmin,
                       const fixed vmax) :
    ZeroFinder(max(fixed(1), vmin - head_wind), vmax - head_wind,
               fixed(TOLERANCE_POLAR_DOLPHIN)),
    polar(_polar),
    m_net_sink_rate(net_sink_rate),
    m_head_wind(head_wind)
  {
  }

  /**
   * Glide ratio over ground function
   *
   * @param V Speed over ground (m/s)
   *
   * @return MacCready-adjusted inverse glide ratio over ground
   */
  fixed
  f(const fixed V)
  {
    return (polar.MSinkRate(V + m_head_wind) + m_net_sink_rate) / V;
  }

  /**
   * Find best speed to fly
   *
   * @param Vstart Initial search speed (m/s)
   *
   * @return Speed to fly (m/s)
   */
  fixed
  solve(const fixed Vstart)
  {
    fixed Vopt = find_min(Vstart);
    return Vopt + m_head_wind;
  }
};

fixed
GlidePolar::SpeedToFly(const AircraftState &state,
    const GlideResult &solution, const bool block_stf) const
{
  fixed V_stf;
  const fixed g_scaling (block_stf ? fixed(1) : sqrt(fabs(state.g_load))); 

  if (!block_stf && (state.netto_vario > mc + Smin)) {
    // stop to climb
    V_stf = Vmin;
  } else {
    const fixed head_wind(!positive(GetMC()) && solution.IsDefined()
                          ? solution.head_wind
                          : fixed(0));
    const fixed stf_sink_rate (block_stf ? fixed(0) : -state.netto_vario);

    GlidePolarSpeedToFly gp_stf(*this, stf_sink_rate, head_wind, Vmin, Vmax);
    V_stf = gp_stf.solve(Vmax);
  }

  return max(Vmin, V_stf*g_scaling);
}

fixed
GlidePolar::GetTotalMass() const
{
  return dry_mass + GetBallastLitres();
}

fixed
GlidePolar::GetWingLoading() const
{
  if (positive(wing_area))
    return GetTotalMass() / wing_area;

  return fixed(0);
}

fixed
GlidePolar::GetBallastLitres() const
{
  return ballast;
}

bool
GlidePolar::IsBallastable() const
{
  return positive(ballast_ratio);
}

static fixed
FRiskFunction(const fixed x, const fixed k)
{
  return fixed(2) / (fixed(1) + exp(-x * k)) - fixed(1);
}

fixed
GlidePolar::GetRiskMC(fixed height_fraction, const fixed riskGamma) const
{
#define fixed_low_limit fixed(0.1)
#define fixed_up_limit fixed(0.9)

  height_fraction = Clamp(height_fraction, fixed(0), fixed(1));

  if (riskGamma < fixed_low_limit)
    return mc;
  else if (riskGamma > fixed_up_limit)
    return height_fraction * mc;

  const fixed k = fixed(1) / sqr(riskGamma) - fixed(1);
  return mc * FRiskFunction(height_fraction, k) / FRiskFunction(fixed(1), k);
}

fixed
GlidePolar::GetBestGlideRatioSpeed(fixed head_wind) const
{
  assert(polar.IsValid());

  fixed s = sqr(head_wind) +
    (mc + polar.c + polar.b * head_wind) / polar.a;
  if (negative(s))
    /* should never happen, but just in case */
    return GetVMax();

  return head_wind + sqrt(s);
}

fixed
GlidePolar::GetVTakeoff() const
{
  return Half(GetVMin());
}

fixed
GlidePolar::GetLDOverGround(Angle track, SpeedVector wind) const
{
  if (wind.IsZero())
    return bestLD;

  const fixed c_theta = (wind.bearing.Reciprocal() - track).cos();

  /* convert the wind speed into some sort of "virtual L/D" to put it
     in relation to the polar's best L/D */
  const fixed wind_ld = wind.norm / GetSBestLD();

  Quadratic q(- Double(wind_ld * c_theta),
              sqr(wind_ld) - sqr(bestLD));

  if (q.Check())
    return max(fixed(0), q.SolutionMax());

  return fixed(0);
}

fixed
GlidePolar::GetLDOverGround(const AircraftState &state) const
{
  return GetLDOverGround(state.track, state.wind);
}

fixed
GlidePolar::GetNextLegEqThermal(fixed current_wind, fixed next_wind) const
{
  assert(polar.IsValid());

  const fixed no_wind_thermal =
      mc - (mc + SbestLD) / VbestLD * current_wind;

  /* calculate coefficients of the polar shifted to the right
     by an amount equal to head wind (ground speed polar) */
  const PolarCoefficients s_polar(polar.a,
                                  polar.b - fixed(2) * next_wind * polar.a,
                                  polar.c + next_wind *
                                  (next_wind * polar.a + polar.b));

  const fixed v_opt = sqrt((s_polar.c + no_wind_thermal) / s_polar.a);
  const fixed s_opt = v_opt * (v_opt * s_polar.a + s_polar.b) + s_polar.c;
  return no_wind_thermal + (s_opt + no_wind_thermal) / v_opt * next_wind;
}
