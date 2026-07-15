// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "STF.hpp"

#include "Atmosphere/AirDensity.hpp"
#include "Computer/Settings.hpp"
#include "NMEA/Aircraft.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Navigation/Aircraft.hpp"

std::optional<double>
ComputeSTFSpeedError(const MoreData &basic,
                     const DerivedInfo &calculated,
                     const ComputerSettings &settings) noexcept
{
  if (!calculated.flight.flying ||
      !basic.airspeed_available ||
      !basic.total_energy_vario_available)
    return std::nullopt;

  GlidePolar polar = settings.polar.glide_polar_task;
  if (!polar.IsValid())
    return std::nullopt;

  if (const auto altitude = basic.GetAnyAltitude())
    polar.SetDensityRatio(AirDensityRatio(*altitude));

  const bool block_stf = settings.features.block_stf_enabled;
  if (!block_stf)
    polar.SetMC(polar.GetRiskMC(
      calculated.common_stats.height_fraction_working,
      settings.task.risk_gamma));

  AircraftState state = ToAircraftState(basic, calculated);
  const double g_load = basic.acceleration.available
    ? basic.acceleration.g_load
    : 1.;

  /* Derive netto from this same TE/airspeed sample.  Do not use a possibly
     device-supplied or GPS-cycle-aged netto value. */
  state.netto_vario = basic.total_energy_vario +
    polar.SinkRate(basic.true_airspeed, g_load);

  const double target = polar.SpeedToFly(
    state, calculated.task_stats.current_leg.solution_remaining, block_stf);
  return target - basic.true_airspeed;
}
