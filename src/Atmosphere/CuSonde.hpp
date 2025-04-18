// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Temperature.hpp"

#include <array>

struct NMEAInfo;
struct DerivedInfo;

/**
 * Namespaces that provides simple estimates of thermal heights from lapse rates
 * derived from temperature trace obtained during flight.
 */
class CuSonde {
public:
  /** Meters between levels */
  static constexpr unsigned HEIGHT_STEP = 100;

  struct Level {
    /** Environmental temperature in K */
    Temperature air_temperature;
    /** DewPoint in K */
    Temperature dewpoint;
    /** Dry temperature in K */
    Temperature dry_temperature;
    /** ThermalIndex in K */
    Temperature thermal_index;

    void UpdateTemps(bool humidity_valid, double humidity,
                     Temperature temperature) noexcept;
    void UpdateThermalIndex(double h_agl,
                            Temperature max_ground_temperature) noexcept;

    /** Has any data */
    bool has_data;
    /** Has dewpoint data */
    bool has_dewpoint;

    /** Estimated ThermalHeight with data of this level */
    double thermal_height;
    /** Estimated CloudBase with data of this level */
    double cloud_base;

    constexpr bool empty() const noexcept {
      return !has_data;
    }

    constexpr bool dewpoint_empty() const noexcept {
      return !has_dewpoint;
    }

    constexpr void Reset() noexcept {
      has_data = false;
      has_dewpoint = false;
    }
  };

  /** Expected temperature maximum on the ground */
  Temperature max_ground_temperature;
  /** Height of ground above MSL */
  double ground_height;
  unsigned short last_level;
  std::array<Level, 100> cslevels;

  /** Estimated ThermailHeight */
  double thermal_height;
  /** Estimated CloudBase */
  double cloud_base;

  void Reset() noexcept;

  void UpdateMeasurements(const NMEAInfo &basic,
                          const DerivedInfo &calculated) noexcept;
  void FindCloudBase(unsigned short level) noexcept;
  void FindThermalHeight(unsigned short level) noexcept;
  void SetForecastTemperature(Temperature temperature) noexcept;
};
