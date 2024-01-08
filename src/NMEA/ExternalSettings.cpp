// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ExternalSettings.hpp"

void
ExternalSettings::Clear()
{
  mac_cready_available.Clear();
  ballast_fraction_available.Clear();
  ballast_overload_available.Clear();
  wing_loading_available.Clear();
  bugs_available.Clear();
  qnh_available.Clear();
  volume_available.Clear();
  has_active_frequency.Clear();
  active_frequency.Clear();
  active_freq_name.clear();
  has_standby_frequency.Clear();
  standby_frequency.Clear();
  standby_freq_name.clear();
  swap_frequencies.Clear();
  has_transponder_code.Clear();
  transponder_code.Clear();
  ballast_litres_available.Clear();
}

void
ExternalSettings::Expire([[maybe_unused]] TimeStamp time) noexcept
{
  /* the settings do not expire, they are only updated with a new
     value */
}

void
ExternalSettings::Complement(const ExternalSettings &add)
{
  if (add.mac_cready_available.Modified(mac_cready_available)) {
    mac_cready = add.mac_cready;
    mac_cready_available = add.mac_cready_available;
  }

  if (add.ballast_fraction_available.Modified(ballast_fraction_available)) {
    ballast_fraction = add.ballast_fraction;
    ballast_fraction_available = add.ballast_fraction_available;
  }

  if (add.wing_loading_available.Modified(wing_loading_available)) {
    wing_loading = add.wing_loading;
    wing_loading_available = add.wing_loading_available;
  }

  if (add.ballast_overload_available.Modified(ballast_overload_available)) {
    ballast_overload = add.ballast_overload;
    ballast_overload_available = add.ballast_overload_available;
  }

  if (add.ballast_litres_available.Modified(ballast_litres_available)) {
     ballast_litres = add.ballast_litres;
     ballast_litres_available = add.ballast_litres_available;
  }

  if (add.bugs_available.Modified(bugs_available)) {
    bugs = add.bugs;
    bugs_available = add.bugs_available;
  }

  if (add.qnh_available.Modified(qnh_available)) {
    qnh = add.qnh;
    qnh_available = add.qnh_available;
  }

  if (add.volume_available.Modified(volume_available)) {
    volume = add.volume;
    volume_available = add.volume_available;
  }

  if (add.has_active_frequency.Modified(has_active_frequency) &&
      add.active_frequency.IsDefined()) {
    has_active_frequency = add.has_active_frequency;
    active_frequency = add.active_frequency;
    active_freq_name = add.active_freq_name;
  }

  if (add.has_standby_frequency.Modified(has_standby_frequency) &&
      add.standby_frequency.IsDefined()) {
    has_standby_frequency = add.has_standby_frequency;
    standby_frequency = add.standby_frequency;
    standby_freq_name = add.standby_freq_name;
  }

  if (add.swap_frequencies.Modified(swap_frequencies)) {
    swap_frequencies = add.swap_frequencies;
  }

  if (add.has_transponder_code.Modified(has_transponder_code) &&
      add.transponder_code.IsDefined()) {
    has_transponder_code = add.has_transponder_code;
    transponder_code = add.transponder_code;
  }

}

void
ExternalSettings::EliminateRedundant(const ExternalSettings &other,
                                     const ExternalSettings &last)
{
  if (mac_cready_available && other.CompareMacCready(mac_cready) &&
      !last.CompareMacCready(mac_cready))
    mac_cready_available.Clear();

  if (ballast_fraction_available &&
      other.CompareBallastFraction(ballast_fraction) &&
      !last.CompareBallastFraction(ballast_fraction))
    ballast_fraction_available.Clear();

  if (ballast_overload_available &&
      other.CompareBallastOverload(ballast_overload) &&
      !last.CompareBallastOverload(ballast_overload))
    ballast_overload_available.Clear();

  if (ballast_litres_available &&
      other.CompareBallastLitres(ballast_litres) &&
      !last.CompareBallastLitres(ballast_litres))
    ballast_litres_available.Clear();

  if (wing_loading_available && other.CompareWingLoading(wing_loading) &&
      !last.CompareWingLoading(wing_loading))
    wing_loading_available.Clear();

  if (bugs_available && other.CompareBugs(bugs) &&
      !last.CompareBugs(bugs))
    bugs_available.Clear();

  if (qnh_available && other.CompareQNH(qnh) && !last.CompareQNH(qnh))
    qnh_available.Clear();

  if (volume_available && other.CompareVolume(volume) &&
      !last.CompareVolume(volume))
    volume_available.Clear();
}

bool
ExternalSettings::ProvideMacCready(double value, TimeStamp time) noexcept
{
  if (value < 0 || value > 20)
    /* failed sanity check */
    return false;

  if (CompareMacCready(value))
    return false;

  mac_cready = value;
  mac_cready_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideBallastLitres(double value, TimeStamp time) noexcept
{
  if (value < 0 || value > 1000)
    /* failed sanity check */
    return false;

  if (CompareBallastLitres(value))
    return false;
  ballast_litres = value;
  ballast_litres_available.Update(time);
  return true;
}


bool
ExternalSettings::ProvideBallastFraction(double value, TimeStamp time) noexcept
{
  if (value < 0 || value > 1)
    /* failed sanity check */
    return false;

  if (CompareBallastFraction(value))
    return false;

  ballast_fraction = value;
  ballast_fraction_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideBallastOverload(double value, TimeStamp time) noexcept
{
  if (value < 0.8 || value > 5)
    /* failed sanity check */
    return false;

  if (CompareBallastOverload(value))
    return false;

  ballast_overload = value;
  ballast_overload_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideWingLoading(double value, TimeStamp time) noexcept
{
  if (value < 1 || value > 200)
    /* failed sanity check */
    return false;

  if (CompareWingLoading(value))
    return false;

  wing_loading = value;
  wing_loading_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideBugs(double value, TimeStamp time) noexcept
{
  if (value < 0.5 || value > 1)
    /* failed sanity check */
    return false;

  if (CompareBugs(value))
    return false;

  bugs = value;
  bugs_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideQNH(AtmosphericPressure value,
                             TimeStamp time) noexcept
{
  if (value.GetHectoPascal() < 500 ||
      value.GetHectoPascal() > 1500)
    /* failed sanity check */
    return false;

  if (CompareQNH(value))
    return false;

  qnh = value;
  qnh_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideVolume(unsigned value, TimeStamp time) noexcept
{
  if (value > 100)
    /* failed sanity check */
    return false;

  if (CompareVolume(value))
    return false;

  volume = value;
  volume_available.Update(time);
  return true;
}
