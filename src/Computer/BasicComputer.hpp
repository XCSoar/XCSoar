// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GroundSpeedComputer.hpp"
#include "FilteredVarioComputer.hpp"
#include "Math/Angle.hpp"

#include <chrono>

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
  FilteredVarioComputer filtered_vario;

  /**
   * Simulator and replay publish a fix about once a second, then
   * merge copies the attitude away.  The yaw rate is the latest
   * sample; bank and pitch ease toward it so the horizon does not
   * step.
   */
  struct PlaybackAttitude {
    Angle yaw_rate = Angle::Zero();
    Angle bank = Angle::Zero();
    Angle pitch = Angle::Zero();
    std::chrono::steady_clock::time_point shown_at{};
    bool shown = false;
  } playback;

  [[nodiscard]] double PlaybackFrame() noexcept;

  void EstimatePlaybackAttitude(MoreData &basic, const MoreData &last,
                                const DerivedInfo &calculated,
                                double frame) noexcept;

  void ComputeDynamics(MoreData &basic, const MoreData &last,
                       const DerivedInfo &calculated,
                       double frame) noexcept;

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
               const DerivedInfo &calculated,
               const ComputerSettings &settings) noexcept;

  [[gnu::pure]]
  bool FilteredVarioSampleUpdated() const noexcept {
    return filtered_vario.SampleUpdated();
  }

  [[gnu::pure]]
  bool FilteredVarioActive() const noexcept {
    return filtered_vario.FilterActive();
  }
};
