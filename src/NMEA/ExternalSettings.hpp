// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"
#include "Atmosphere/Pressure.hpp"
#include "RadioFrequency.hpp"
#include "TransponderCode.hpp"
#include "TransponderMode.hpp"
#include "util/StaticString.hxx"

#include <stdlib.h>
#include <math.h>

/**
 * Settings received from an external device.
 */
struct ExternalSettings {
  Validity mac_cready_available;

  /** MacCready value (m/s) of external device (if available) */
  double mac_cready;

  Validity ballast_fraction_available;

  /** Ballast information (1: full, 0: empty) of external device (if available) */
  double ballast_fraction;

  Validity ballast_overload_available;

  /**
   * Ballast information (1: no ballast, 1.5: ballast = 0.5 times dry mass)
   * of external device (if available)
   */
  double ballast_overload;

  Validity wing_loading_available;

  /**
   * amount of water ballast in kg (or litres) when altered in external device
   * from external device (if available)
   */
  double ballast_litres;

  Validity ballast_litres_available;

  /** Wing loading information (kg/m^2) of external device (if available) */
  double wing_loading;

  Validity bugs_available;

  /** Bugs information (1: clean, 0: dirty) of external device (if available) */
  double bugs;

  Validity qnh_available;

  /** the QNH setting [hPa] */
  AtmosphericPressure qnh;

  Validity volume_available;

  /** the volume of the device [0-100%] */
  unsigned volume;

  Validity elevation_available;

  /** the elevation setting [meters] */
  int elevation;

  /** POLAR data from device */
  Validity polar_coefficients_available;
  double polar_a;
  double polar_b;
  double polar_c;

  Validity polar_load_available;
  /** Polar load (overload) from device POLAR sentence */
  double polar_load;

  Validity polar_reference_mass_available;
  /** Reference mass (polar weight) from device POLAR sentence [kg] */
  double polar_reference_mass;

  Validity polar_maximum_mass_available;
  /** Maximum mass (max weight) from device POLAR sentence [kg] */
  double polar_maximum_mass;

  Validity polar_pilot_weight_available;
  /** Pilot weight from device POLAR sentence [kg] */
  double polar_pilot_weight;

  Validity polar_empty_weight_available;
  /** Empty weight from device POLAR sentence [kg] */
  double polar_empty_weight;

  /** the radio frequencies of the device */
  Validity has_active_frequency;
  RadioFrequency active_frequency;
  StaticString<32> active_freq_name;

  Validity has_standby_frequency;
  RadioFrequency standby_frequency;
  StaticString<32> standby_freq_name;

  Validity swap_frequencies;

  /** Glider identity from device (e.g. declaration H-records) */
  Validity glider_registration_available;
  StaticString<32> glider_registration;

  Validity glider_competition_id_available;
  StaticString<6> glider_competition_id;

  Validity glider_type_available;
  StaticString<32> glider_type;

  /** transponder */
  Validity has_transponder_code;
  TransponderCode transponder_code;
  Validity has_transponder_mode;
  TransponderMode transponder_mode;

  void Clear();
  void Expire(TimeStamp time) noexcept;
  void Complement(const ExternalSettings &add);

  /**
   * Clear attributes that have the same values like those in the
   * "other" object, but only if that is different from the values in
   * "last".
   *
   * This is used by the device code to break the feedback loop: when
   * a setting is sent to the device, and the device reports back the
   * new setting, it is ignored, by calling this method.
   */
  void EliminateRedundant(const ExternalSettings &other,
                          const ExternalSettings &last);

  /**
   * Compare the MacCready setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareMacCready(double value) const {
    return mac_cready_available && fabs(mac_cready - value) <= 0.05;
  }

  /**
   * Compare the ballast fraction setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBallastFraction(double value) const {
    return ballast_fraction_available &&
      fabs(ballast_fraction - value) <= 0.01;
  }

  /**
   * Compare the ballast overload setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBallastOverload(double value) const {
    return ballast_overload_available &&
      fabs(ballast_overload - value) <= 0.01;
  }

  /**
   * Compare the absolure ballast in kg (litres) setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBallastLitres(double value) const {
    return ballast_litres_available &&
      fabs(ballast_litres - value) <= 0.05;
  }

  /**
   * Compare the wing loading setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareWingLoading(double value) const {
    return wing_loading_available &&
      fabs(wing_loading - value) <= 0.5;
  }

  /**
   * Compare the bugs setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBugs(double value) const {
    return bugs_available && fabs(bugs - value) <= 0.01;
  }

  /**
   * Compare the QNH setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareQNH(AtmosphericPressure value) const {
    return qnh_available &&
      fabs(qnh.GetHectoPascal() - value.GetHectoPascal()) <= 0.5;
  }

  /**
   * Compare the volume setting with the specified value.
   *
   * @return true if the current setting is the same (within 3% difference),
   * false if the value is different or if there is no value
   */
  bool CompareVolume(unsigned value) const {
    return volume_available && abs(int(volume) - int(value)) < 3;
  }

  /**
   * Compare the elevation setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareElevation(int value) const {
    return elevation_available && abs(elevation - value) <= 1;
  }

  /**
   * Sets a new MacCready value, but updates the time stamp only if
   * the value has changed.
   *
   * @return true if the value and the time stamp have been updated
   */
  bool ProvideMacCready(double value, TimeStamp time) noexcept;
  bool ProvideBallastFraction(double value, TimeStamp time) noexcept;
  bool ProvideBallastOverload(double value, TimeStamp time) noexcept;
  bool ProvideBallastLitres(double value, TimeStamp time) noexcept;
  bool ProvideWingLoading(double value, TimeStamp time) noexcept;
  bool ProvideBugs(double value, TimeStamp time) noexcept;
  bool ProvideQNH(AtmosphericPressure value, TimeStamp time) noexcept;
  bool ProvideVolume(unsigned value, TimeStamp time) noexcept;
  bool ProvideElevation(int value, TimeStamp time) noexcept;
  bool ProvidePolarCoefficients(double a, double b, double c, TimeStamp time) noexcept;
  bool ProvidePolarLoad(double value, TimeStamp time) noexcept;
  bool ProvidePolarReferenceMass(double value, TimeStamp time) noexcept;
  bool ProvidePolarMaximumMass(double value, TimeStamp time) noexcept;
  bool ProvidePolarPilotWeight(double value, TimeStamp time) noexcept;
  bool ProvidePolarEmptyWeight(double value, TimeStamp time) noexcept;
  bool ProvideGliderRegistration(const char *value, TimeStamp time) noexcept;
  bool ProvideGliderCompetitionId(const char *value, TimeStamp time) noexcept;
  bool ProvideGliderType(const char *value, TimeStamp time) noexcept;

  bool ComparePolarCoefficients(double a, double b, double c) const {
    return polar_coefficients_available &&
      fabs(polar_a - a) <= 0.0001 &&
      fabs(polar_b - b) <= 0.0001 &&
      fabs(polar_c - c) <= 0.0001;
  }

  bool ComparePolarLoad(double value) const {
    return polar_load_available && fabs(polar_load - value) <= 0.01;
  }

  bool ComparePolarReferenceMass(double value) const {
    return polar_reference_mass_available &&
      fabs(polar_reference_mass - value) <= 0.1;
  }

  bool ComparePolarMaximumMass(double value) const {
    return polar_maximum_mass_available &&
      fabs(polar_maximum_mass - value) <= 0.1;
  }

  bool ComparePolarPilotWeight(double value) const {
    return polar_pilot_weight_available &&
      fabs(polar_pilot_weight - value) <= 0.1;
  }

  bool ComparePolarEmptyWeight(double value) const {
    return polar_empty_weight_available &&
      fabs(polar_empty_weight - value) <= 0.1;
  }
};
