// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlidePolar.hpp"
#include "GlideState.hpp"
#include "GlideResult.hpp"
#include "Math/ZeroFinder.hpp"
#include "Math/Quadratic.hpp"
#include "Math/Util.hpp"
#include "Navigation/Aircraft.hpp"

#include <algorithm>

#include <cassert>

GlidePolar::GlidePolar(const double _mc, const double _bugs,
                       const double _ballast) noexcept
  :mc(_mc),
   bugs(_bugs),
   ballast(_ballast),
   cruise_efficiency(1),
   VbestLD(0),
   Vmax(75),
   Vmin(0),
   reference_polar(0.00157, -0.0734, 1.48),
   ballast_ratio(0.3),
   reference_mass(300),
   empty_mass(reference_mass),
   crew_mass(90.),
   wing_area(0)
{
  Update();

  // Calculate inv_mc
  SetMC(mc);
}

void
GlidePolar::Update() noexcept
{
  assert(bugs > 0);

  if (!reference_polar.IsValid()) {
    Vmin = Vmax = 0;
    return;
  }

  const auto loading_factor = sqrt(GetTotalMass() / reference_mass);
  const auto inv_bugs = 1. / bugs;

  polar.a = inv_bugs * reference_polar.a / loading_factor;
  polar.b = inv_bugs * reference_polar.b;
  polar.c = inv_bugs * reference_polar.c * loading_factor;

  assert(polar.IsValid());

  UpdateSMax();
  UpdateSMin();
}

void
GlidePolar::UpdateSMax() noexcept
{
  assert(polar.IsValid());

  Smax = SinkRate(Vmax);
}

void
GlidePolar::SetBugs(const double clean) noexcept
{
  assert(clean > 0 && clean <= 1);
  bugs = clean;
  Update();
}

void
GlidePolar::SetBallast(const double bal) noexcept
{
  assert(bal >= 0);
  SetBallastLitres(bal * ballast_ratio * reference_mass);
}

void
GlidePolar::SetBallastLitres(const double litres) noexcept
{
  assert(litres >= 0);
  ballast = litres;
  Update();
}

void
GlidePolar::SetMC(const double _mc) noexcept
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
GlidePolar::MSinkRate(const double V) const noexcept
{
  return SinkRate(V) + mc;
}

double
GlidePolar::SinkRate(const double V) const noexcept
{
  assert(polar.IsValid());

  return V * (V * polar.a + polar.b) + polar.c;
}

double
GlidePolar::SinkRate(const double V, const double n) const noexcept
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
  GlidePolarVopt(const GlidePolar &_polar, const double vmin, const double vmax) noexcept
    :ZeroFinder(vmin, vmax, TOLERANCE_BEST_LD),
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
  double f(const double V) noexcept override {
    return -V/polar.MSinkRate(V);
  }
};
#endif

void
GlidePolar::UpdateBestLD() noexcept
{
#if 0
  // this method to be used if polar is not parabolic
  GlidePolarVopt gpvopt(*this, Vmin, Vmax);
  VbestLD = gpvopt.find_min(VbestLD);
#else
  assert(polar.IsValid());
  assert(mc >= 0);

  VbestLD = std::clamp(sqrt((polar.c + mc) / polar.a), Vmin, Vmax);
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
  GlidePolarMinSink(const GlidePolar &_polar, const double vmax) noexcept
    :ZeroFinder(1, vmax, TOLERANCE_MIN_SINK),
     polar(_polar)
  {
  }

  double f(const double V) noexcept override {
    return polar.SinkRate(V);
  }
};
#endif

void 
GlidePolar::UpdateSMin() noexcept
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
GlidePolar::IsGlidePossible(const GlideState &task) const noexcept
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
  static constexpr double TOLERANCE_DOLPHIN = 0.0001;

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
               TOLERANCE_DOLPHIN),
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
  double f(const double V) noexcept override {
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
GlidePolar::SpeedToFly(const double stf_sink_rate,
                       const double head_wind) const noexcept
{
  assert(IsValid());
  GlidePolarSpeedToFly gp_stf(*this, stf_sink_rate, head_wind, Vmin, Vmax);
  return gp_stf.solve(Vmax);
}

double
GlidePolar::SpeedToFly(const AircraftState &state,
                       const GlideResult &solution,
                       const bool block_stf) const noexcept
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
GlidePolar::GetTotalMass() const noexcept
{
  return empty_mass + crew_mass + GetBallastLitres();
}

double
GlidePolar::GetWingLoading() const noexcept
{
  if (wing_area > 0)
    return GetTotalMass() / wing_area;

  return 0;
}

[[gnu::const]]
static double
FRiskFunction(const double x, const double k) noexcept
{
  return 2 / (1 + exp(-x * k)) - 1;
}

double
GlidePolar::GetRiskMC(double height_fraction,
                      const double riskGamma) const noexcept
{
  constexpr double low_limit = 0.1;
  constexpr double up_limit = 0.9;

  height_fraction = std::clamp(height_fraction, 0., 1.);

  if (riskGamma < low_limit)
    return mc;
  else if (riskGamma > up_limit)
    return height_fraction * mc;

  const auto k = 1. / (riskGamma * riskGamma) - 1;
  return mc * FRiskFunction(height_fraction, k) / FRiskFunction(1, k);
}

double
GlidePolar::GetBestGlideRatioSpeed(double head_wind) const noexcept
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
GlidePolar::GetVTakeoff() const noexcept
{
  return GetVMin() / 2;
}

double
GlidePolar::GetLDOverGround(Angle track, SpeedVector wind) const noexcept
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
GlidePolar::GetLDOverGround(const AircraftState &state) const noexcept
{
  return GetLDOverGround(state.track, state.wind);
}

double
GlidePolar::GetNextLegEqThermal(double current_wind,
                                double next_wind) const noexcept
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


double GlidePolar::GetAverageSpeed() const noexcept
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
