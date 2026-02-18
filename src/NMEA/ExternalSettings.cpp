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
  elevation_available.Clear();
  has_active_frequency.Clear();
  active_frequency.Clear();
  active_freq_name.clear();
  has_standby_frequency.Clear();
  standby_frequency.Clear();
  standby_freq_name.clear();
  swap_frequencies.Clear();
  glider_registration_available.Clear();
  glider_registration.clear();
  glider_competition_id_available.Clear();
  glider_competition_id.clear();
  glider_type_available.Clear();
  glider_type.clear();
  has_transponder_code.Clear();
  transponder_code.Clear();
  transponder_mode.Clear();
  ballast_litres_available.Clear();
  polar_coefficients_available.Clear();
  polar_load_available.Clear();
  polar_reference_mass_available.Clear();
  polar_maximum_mass_available.Clear();
  polar_pilot_weight_available.Clear();
  polar_empty_weight_available.Clear();
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

  if (add.elevation_available.Modified(elevation_available)) {
    elevation = add.elevation;
    elevation_available = add.elevation_available;
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

  if (add.glider_registration_available.Modified(glider_registration_available) &&
      !add.glider_registration.empty()) {
    glider_registration_available = add.glider_registration_available;
    glider_registration = add.glider_registration;
  }

  if (add.glider_competition_id_available.Modified(glider_competition_id_available) &&
      !add.glider_competition_id.empty()) {
    glider_competition_id_available = add.glider_competition_id_available;
    glider_competition_id = add.glider_competition_id;
  }

  if (add.glider_type_available.Modified(glider_type_available) &&
      !add.glider_type.empty()) {
    glider_type_available = add.glider_type_available;
    glider_type = add.glider_type;
  }

  if (add.has_transponder_code.Modified(has_transponder_code) &&
      add.transponder_code.IsDefined()) {
    has_transponder_code = add.has_transponder_code;
    transponder_code = add.transponder_code;
  }

  if (add.has_transponder_mode.Modified(has_transponder_mode) &&
      add.transponder_mode.IsDefined()) {
    has_transponder_mode = add.has_transponder_mode;
    transponder_mode = add.transponder_mode;
  }

  if (add.polar_coefficients_available.Modified(polar_coefficients_available)) {
    polar_a = add.polar_a;
    polar_b = add.polar_b;
    polar_c = add.polar_c;
    polar_coefficients_available = add.polar_coefficients_available;
  }

  if (add.polar_load_available.Modified(polar_load_available)) {
    polar_load = add.polar_load;
    polar_load_available = add.polar_load_available;
  }

  if (add.polar_reference_mass_available.Modified(polar_reference_mass_available)) {
    polar_reference_mass = add.polar_reference_mass;
    polar_reference_mass_available = add.polar_reference_mass_available;
  }

  if (add.polar_maximum_mass_available.Modified(polar_maximum_mass_available)) {
    polar_maximum_mass = add.polar_maximum_mass;
    polar_maximum_mass_available = add.polar_maximum_mass_available;
  }

  if (add.polar_pilot_weight_available.Modified(polar_pilot_weight_available)) {
    polar_pilot_weight = add.polar_pilot_weight;
    polar_pilot_weight_available = add.polar_pilot_weight_available;
  }

  if (add.polar_empty_weight_available.Modified(polar_empty_weight_available)) {
    polar_empty_weight = add.polar_empty_weight;
    polar_empty_weight_available = add.polar_empty_weight_available;
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

  if (elevation_available && other.CompareElevation(elevation) &&
      !last.CompareElevation(elevation))
    elevation_available.Clear();

  if (polar_coefficients_available &&
      other.ComparePolarCoefficients(polar_a, polar_b, polar_c) &&
      !last.ComparePolarCoefficients(polar_a, polar_b, polar_c))
    polar_coefficients_available.Clear();

  if (polar_load_available &&
      other.ComparePolarLoad(polar_load) &&
      !last.ComparePolarLoad(polar_load))
    polar_load_available.Clear();

  if (polar_reference_mass_available &&
      other.ComparePolarReferenceMass(polar_reference_mass) &&
      !last.ComparePolarReferenceMass(polar_reference_mass))
    polar_reference_mass_available.Clear();

  if (polar_maximum_mass_available &&
      other.ComparePolarMaximumMass(polar_maximum_mass) &&
      !last.ComparePolarMaximumMass(polar_maximum_mass))
    polar_maximum_mass_available.Clear();

  if (polar_pilot_weight_available &&
      other.ComparePolarPilotWeight(polar_pilot_weight) &&
      !last.ComparePolarPilotWeight(polar_pilot_weight))
    polar_pilot_weight_available.Clear();

  if (polar_empty_weight_available &&
      other.ComparePolarEmptyWeight(polar_empty_weight) &&
      !last.ComparePolarEmptyWeight(polar_empty_weight))
    polar_empty_weight_available.Clear();
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

bool
ExternalSettings::ProvideElevation(int value, TimeStamp time) noexcept
{
  if (value < -500 || value > 9000)
    /* failed sanity check */
    return false;

  if (CompareElevation(value))
    return false;

  elevation = value;
  elevation_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvidePolarCoefficients(double a, double b, double c, TimeStamp time) noexcept
{
  /* Sanity checks for polar coefficients */
  if (a <= 0 || c <= 0 || b >= 0)
    return false;

  /* Check if values have changed */
  if (polar_coefficients_available &&
      fabs(polar_a - a) <= 0.0001 &&
      fabs(polar_b - b) <= 0.0001 &&
      fabs(polar_c - c) <= 0.0001)
    return false;

  polar_a = a;
  polar_b = b;
  polar_c = c;
  polar_coefficients_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvidePolarLoad(double value, TimeStamp time) noexcept
{
  if (value < 0 || value > 100)
    /* failed sanity check — wing loading in kg/m² */
    return false;

  if (polar_load_available && fabs(polar_load - value) <= 0.01)
    return false;

  polar_load = value;
  polar_load_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvidePolarReferenceMass(double value, TimeStamp time) noexcept
{
  if (value < 100 || value > 1000)
    /* failed sanity check */
    return false;

  if (polar_reference_mass_available && fabs(polar_reference_mass - value) <= 0.1)
    return false;

  polar_reference_mass = value;
  polar_reference_mass_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvidePolarMaximumMass(double value, TimeStamp time) noexcept
{
  if (value < 100 || value > 1500)
    /* failed sanity check */
    return false;

  if (polar_maximum_mass_available && fabs(polar_maximum_mass - value) <= 0.1)
    return false;

  polar_maximum_mass = value;
  polar_maximum_mass_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvidePolarPilotWeight(double value, TimeStamp time) noexcept
{
  if (value < 0 || value > 200)
    /* failed sanity check */
    return false;

  if (ComparePolarPilotWeight(value))
    return false;

  polar_pilot_weight = value;
  polar_pilot_weight_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvidePolarEmptyWeight(double value, TimeStamp time) noexcept
{
  if (value < 0 || value > 1000)
    /* failed sanity check */
    return false;

  if (ComparePolarEmptyWeight(value))
    return false;

  polar_empty_weight = value;
  polar_empty_weight_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideGliderRegistration(const char *value,
                                            TimeStamp time) noexcept
{
  if (value == nullptr || *value == '\0')
    return false;

  if (glider_registration_available && glider_registration.equals(value))
    return false;

  glider_registration.SetUTF8(value);
  glider_registration_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideGliderCompetitionId(const char *value,
                                             TimeStamp time) noexcept
{
  if (value == nullptr || *value == '\0')
    return false;

  if (glider_competition_id_available &&
      glider_competition_id.equals(value))
    return false;

  glider_competition_id.SetUTF8(value);
  glider_competition_id_available.Update(time);
  return true;
}

bool
ExternalSettings::ProvideGliderType(const char *value,
                                    TimeStamp time) noexcept
{
  if (value == nullptr || *value == '\0')
    return false;

  if (glider_type_available && glider_type.equals(value))
    return false;

  glider_type.SetUTF8(value);
  glider_type_available.Update(time);
  return true;
}
