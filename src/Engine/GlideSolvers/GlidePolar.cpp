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

#include "GlidePolar.hpp"
#include "GlideState.hpp"
#include "GlideResult.hpp"
#include "Math/ZeroFinder.hpp"
#include "Math/Quadratic.hpp"
#include "Math/Util.hpp"
#include "Util/Tolerances.hpp"
#include "Util/Clamp.hpp"
#include "Navigation/Aircraft.hpp"

#include <algorithm>

#include <assert.h>

GlidePolar::GlidePolar(const double _mc, const double _bugs, const double _ballast)
  :mc(_mc),
   bugs(_bugs),
   ballast(_ballast),
   cruise_efficiency(1),
   VbestLD(0),
   Vmax(75),
   Vmin(0),
   ideal_polar(0.00157, -0.0734, 1.48),
   ballast_ratio(0.3),
   reference_mass(300),
   dry_mass(reference_mass),
   wing_area(0)
{
  Update();

  // Calculate inv_mc
  SetMC(mc);
}

void
GlidePolar::Update()
{
  assert(bugs > 0);

  if (!ideal_polar.IsValid()) {
    Vmin = Vmax = 0;
    return;
  }

  const auto loading_factor = sqrt(GetTotalMass() / reference_mass);
  const auto inv_bugs = 1. / bugs;

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
GlidePolar::SetBugs(const double clean)
{
  assert(clean > 0 && clean <= 1);
  bugs = clean;
  Update();
}

void
GlidePolar::SetBallast(const double bal)
{
  assert(bal >= 0);
  SetBallastLitres(bal * ballast_ratio * reference_mass);
}

void
GlidePolar::SetBallastLitres(const double litres)
{
  assert(litres >= 0);
  ballast = litres;
  Update();
}

void
GlidePolar::SetMC(const double _mc)
{
  mc = _mc;

  if (mc > 0)
    inv_mc = 1. / mc;
  else
    inv_mc = 0;

  if (IsValid())
    UpdateBestLD();
}

double
GlidePolar::MSinkRate(const double V) const
{
  return SinkRate(V) + mc;
}

double
GlidePolar::SinkRate(const double V) const
{
  assert(polar.IsValid());

  return V * (V * polar.a + polar.b) + polar.c;
}

double
GlidePolar::SinkRate(const double V, const double n) const
{
  const auto w0 = SinkRate(V);
  const auto vl = VbestLD / std::max(VbestLD / 2, V);
  return std::max(0.,
                  w0 + (0.5 * V / bestLD) * (n * n - 1) * vl * vl);
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
  GlidePolarVopt(const GlidePolar &_polar, const double vmin, const double vmax)
    :ZeroFinder(vmin, vmax, TOLERANCE_POLAR_BESTLD),
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
  double f(const double V) {
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
  assert(mc >= 0);

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
  GlidePolarMinSink(const GlidePolar &_polar, const double vmax)
    :ZeroFinder(1, vmax, TOLERANCE_POLAR_MINSINK),
     polar(_polar)
  {
  }

  double f(const double V) {
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

  Vmin = std::min(Vmax, -0.5 * polar.b / polar.a);
  Smin = SinkRate(Vmin);
#endif

  UpdateBestLD();
}

bool
GlidePolar::IsGlidePossible(const GlideState &task) const
{
  if (task.altitude_difference <= 0)
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
class GlidePolarSpeedToFly final : public ZeroFinder {
  const GlidePolar &polar;
  const double m_net_sink_rate;
  const double m_head_wind;

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
  GlidePolarSpeedToFly(const GlidePolar &_polar, const double net_sink_rate,
                       const double head_wind, const double vmin,
                       const double vmax) :
    ZeroFinder(std::max(1., vmin - head_wind), vmax - head_wind,
               TOLERANCE_POLAR_DOLPHIN),
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
  double f(const double V) {
    return (polar.MSinkRate(V + m_head_wind) + m_net_sink_rate) / V;
  }

  /**
   * Find best speed to fly
   *
   * @param Vstart Initial search speed (m/s)
   *
   * @return Speed to fly (m/s)
   */
  double solve(const double Vstart) {
    auto Vopt = find_min(Vstart);
    return Vopt + m_head_wind;
  }
};

double
GlidePolar::SpeedToFly(const double stf_sink_rate, const double head_wind) const
{
  assert(IsValid());
  GlidePolarSpeedToFly gp_stf(*this, stf_sink_rate, head_wind, Vmin, Vmax);
  return gp_stf.solve(Vmax);
}

double
GlidePolar::SpeedToFly(const AircraftState &state,
                       const GlideResult &solution, const bool block_stf) const
{
  assert(IsValid());

  double V_stf;
  const auto g_scaling = block_stf
    ? 1.
    : sqrt(fabs(state.g_load));

  if (!block_stf && (state.netto_vario > mc + Smin)) {
    // stop to climb
    V_stf = Vmin;
  } else {
    const auto head_wind = GetMC() <= 0 && solution.IsDefined()
      ? solution.head_wind
      : 0.;
    const auto stf_sink_rate = block_stf
      ? 0.
      : -state.netto_vario;

    V_stf = SpeedToFly(stf_sink_rate, head_wind);
  }

  return std::max(Vmin, V_stf * g_scaling);
}

double
GlidePolar::GetTotalMass() const
{
  return dry_mass + GetBallastLitres();
}

double
GlidePolar::GetWingLoading() const
{
  if (wing_area > 0)
    return GetTotalMass() / wing_area;

  return 0;
}

double
GlidePolar::GetBallastLitres() const
{
  return ballast;
}

bool
GlidePolar::IsBallastable() const
{
  return ballast_ratio > 0;
}

static double
FRiskFunction(const double x, const double k)
{
  return 2 / (1 + exp(-x * k)) - 1;
}

double
GlidePolar::GetRiskMC(double height_fraction, const double riskGamma) const
{
  constexpr double low_limit = 0.1;
  constexpr double up_limit = 0.9;

  height_fraction = Clamp(height_fraction, 0., 1.);

  if (riskGamma < low_limit)
    return mc;
  else if (riskGamma > up_limit)
    return height_fraction * mc;

  const auto k = 1. / (riskGamma * riskGamma) - 1;
  return mc * FRiskFunction(height_fraction, k) / FRiskFunction(1, k);
}

double
GlidePolar::GetBestGlideRatioSpeed(double head_wind) const
{
  assert(polar.IsValid());

  auto s = head_wind * head_wind +
    (mc + polar.c + polar.b * head_wind) / polar.a;
  if (s < 0)
    /* should never happen, but just in case */
    return GetVMax();

  return head_wind + sqrt(s);
}

double
GlidePolar::GetVTakeoff() const
{
  return GetVMin() / 2;
}

double
GlidePolar::GetLDOverGround(Angle track, SpeedVector wind) const
{
  if (wind.IsZero())
    return bestLD;

  const auto c_theta = (wind.bearing.Reciprocal() - track).cos();

  /* convert the wind speed into some sort of "virtual L/D" to put it
     in relation to the polar's best L/D */
  const auto wind_ld = wind.norm / GetSBestLD();

  Quadratic q(-2 * wind_ld * c_theta,
              Square(wind_ld) - Square(bestLD));

  if (q.Check())
    return std::max(0., q.SolutionMax());

  return 0;
}

double
GlidePolar::GetLDOverGround(const AircraftState &state) const
{
  return GetLDOverGround(state.track, state.wind);
}

double
GlidePolar::GetNextLegEqThermal(double current_wind, double next_wind) const
{
  assert(polar.IsValid());

  const auto no_wind_thermal =
      mc - (mc + SbestLD) / VbestLD * current_wind;

  /* calculate coefficients of the polar shifted to the right
     by an amount equal to head wind (ground speed polar) */
  const PolarCoefficients s_polar(polar.a,
                                  polar.b - 2 * next_wind * polar.a,
                                  polar.c + next_wind *
                                  (next_wind * polar.a + polar.b));

  const auto v_opt = sqrt((s_polar.c + no_wind_thermal) / s_polar.a);
  const auto s_opt = v_opt * (v_opt * s_polar.a + s_polar.b) + s_polar.c;
  return no_wind_thermal + (s_opt + no_wind_thermal) / v_opt * next_wind;
}


double GlidePolar::GetAverageSpeed() const
{
  const double m = GetMC();
  if (m>0) {
    const double v = GetVBestLD();
    const double d_s = GetSBestLD();
    const double rho = d_s/m;
    return v/(1+rho);
  } else
    return 0;
}
