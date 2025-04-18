// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GroundSpeedComputer.hpp"

struct MoreData;
struct DerivedInfo;
class AtmosphericPressure;
struct FeaturesSettings;
struct ComputerSettings;

/**
 * A computer which adds missing values to #NEMAInfo.  It performs
 * simple and fast calculations after every GPS update, cheap enough
 * to run outside of the #CalculationThread.
 */
class BasicComputer {
  GroundSpeedComputer ground_speed;

public:
  /**
   * Fill the missing attributes with a fallback.
   */
  void Fill(MoreData &data, const AtmosphericPressure qnh,
            const FeaturesSettings &features) noexcept;

  void Fill(MoreData &data,
            const ComputerSettings &settings_computer) noexcept;

  /**
   * Runs all calculations.
   *
   * @param data the current sensor data structure
   * @param last the previous sensor data structure
   * @param last_gps the previous GPS fix
   * @param calculations the most up-to-date version of calculated values
   */
  void Compute(MoreData &data, const MoreData &last, const MoreData &last_gps,
               const DerivedInfo &calculated) noexcept;
};
