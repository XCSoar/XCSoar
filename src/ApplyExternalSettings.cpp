// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ApplyExternalSettings.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "ActionInterface.hpp"
#include "Device/MultipleDevices.hpp"

static bool
BallastProcessTimer() noexcept
{
  bool modified = false;

  static Validity last_fraction, last_overload;
  const ExternalSettings &settings = CommonInterface::Basic().settings;
  const Plane &plane = CommonInterface::GetComputerSettings().plane;
  const GlidePolar &polar = CommonInterface::GetComputerSettings().polar.glide_polar_task;

  if (settings.ballast_fraction_available.Modified(last_fraction)) {
    ActionInterface::SetBallast(settings.ballast_fraction, false);
    modified = true;
  }

  last_fraction = settings.ballast_fraction_available;

  if (settings.ballast_overload_available.Modified(last_overload) &&
      settings.ballast_overload >= 0.8 && plane.max_ballast > 0) {
    auto fraction =
        ((settings.ballast_overload * plane.polar_shape.reference_mass) -
         polar.GetCrewMass() - plane.empty_mass) /
        plane.max_ballast;
    ActionInterface::SetBallast(fraction, false);
    modified = true;
  }

  last_overload = settings.ballast_overload_available;

  return modified;
}

static bool
BallastLitresProcessTimer()
{
  bool modified = false;
  static Validity last_ballast_litres_validity;
  const NMEAInfo &basic = CommonInterface::Basic();
  if(basic.settings.ballast_litres_available.Modified(last_ballast_litres_validity)){
    PolarSettings &polar(CommonInterface::SetComputerSettings().polar);
    polar.glide_polar_task.SetBallastLitres(basic.settings.ballast_litres);
    modified = true;
  }
  last_ballast_litres_validity = basic.settings.ballast_litres_available;
  return modified;
}

static bool
BugsProcessTimer() noexcept
{
  bool modified = false;

  static Validity last;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (settings.bugs_available.Modified(last)) {
    ActionInterface::SetBugs(settings.bugs, false);
    modified = true;
  }

  last = settings.bugs_available;

  return modified;
}

static bool
QNHProcessTimer(OperationEnvironment &env) noexcept
{
  bool modified = false;

  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.settings.qnh_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = basic.settings.qnh;
    settings_computer.pressure_available = basic.settings.qnh_available;
    modified = true;
  }

  if (calculated.pressure_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = calculated.pressure;
    settings_computer.pressure_available = calculated.pressure_available;

    if (backend_components->devices)
      backend_components->devices->PutQNH(settings_computer.pressure, env);

    modified = true;
  }

  return modified;
}

static bool
MacCreadyProcessTimer() noexcept
{
  bool modified = false;

  static ExternalSettings last_external_settings;
  static Validity last_auto_mac_cready;

  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (last_auto_mac_cready.Modified(calculated.auto_mac_cready_available)) {
    /* time warp, reset */
    last_auto_mac_cready.Clear();
    modified = true;
  }

  if (basic.settings.mac_cready_available.Modified(last_external_settings.mac_cready_available)) {
    ActionInterface::SetMacCready(basic.settings.mac_cready, false);
    modified = true;
  } else if (calculated.auto_mac_cready_available.Modified(last_auto_mac_cready)) {
    last_auto_mac_cready = calculated.auto_mac_cready_available;
    ActionInterface::SetMacCready(calculated.auto_mac_cready);
    modified = true;
  }

  last_external_settings = basic.settings;

  return modified;
}

static bool
RadioProcess() noexcept
{
  bool modified = false;

  const NMEAInfo &basic = CommonInterface::Basic();

  static Validity last_active_frequency;
  static Validity last_standby_frequency;
  static Validity last_swap_frequencies;

  if (basic.settings.has_active_frequency.Modified(last_active_frequency)) {
    ActionInterface::SetActiveFrequency(basic.settings.active_frequency, basic.settings.active_freq_name, false);
    last_active_frequency = basic.settings.has_active_frequency;
    modified = true;
  }

  if (basic.settings.has_standby_frequency.Modified(last_standby_frequency)) {
    ActionInterface::SetStandbyFrequency(basic.settings.standby_frequency, basic.settings.standby_freq_name, false);
    last_standby_frequency = basic.settings.has_standby_frequency;
    modified = true;
  }

  if (basic.settings.swap_frequencies.Modified(last_swap_frequencies)) {
    ActionInterface::ExchangeRadioFrequencies(false);
    last_swap_frequencies = basic.settings.swap_frequencies;
  }

  return modified;
}

static bool
TransponderProcess() noexcept
{
  bool modified = false;

  const NMEAInfo &basic = CommonInterface::Basic();

  static Validity last_transponder_code;

  if (basic.settings.has_transponder_code.Modified(last_transponder_code)) {
    ActionInterface::SetTransponderCode(basic.settings.transponder_code, false);
    last_transponder_code = basic.settings.has_transponder_code;
    modified = true;
  }

  return modified;
}

bool
ApplyExternalSettings(OperationEnvironment &env) noexcept
{
  bool modified = false;
  modified |= BallastLitresProcessTimer();
  modified |= BallastProcessTimer();
  modified |= BugsProcessTimer();
  modified |= QNHProcessTimer(env);
  modified |= MacCreadyProcessTimer();
  modified |= RadioProcess();
  modified |= TransponderProcess();
  return modified;
}
