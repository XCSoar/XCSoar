// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

#include <vector>
#include <functional>

/**
 * Manages automatic switching of XCTherm forecast layers (altitude)
 * and time steps based on current flight data.
 *
 * Altitude: switches layer when GPS/baro altitude crosses the midpoint
 * between two loaded layers, with a 20-second hysteresis.
 *
 * Time: switches forecast time at :45 of each hour.
 *
 * Manual override: stepping manually pauses auto for that axis.
 * Auto resumes when the next threshold is crossed, or immediately
 * when the user taps the centre label in the cursor bar.
 */
class XCThermAutoSwitch {
public:
  struct LayerInfo {
    unsigned index;        // index into the LAYERS array
    unsigned altitude_m;   // altitude in meters
    bool is_agl;
  };

  using SwitchCallback = std::function<void(unsigned layer_index)>;
  using TimeSwitchCallback = std::function<void(unsigned utc_hour)>;

private:
  bool enabled = true;

  /* --- Altitude auto-switch --- */
  std::vector<LayerInfo> loaded_layers;
  std::vector<double> thresholds; // midpoints between loaded layers (AMSL only)
  int current_layer_pos = -1;     // position in loaded_layers

  // Hysteresis state
  bool threshold_pending = false;
  int pending_target_pos = -1;
  TimeStamp threshold_crossed_at = TimeStamp::Undefined();
  static constexpr double HYSTERESIS_SECONDS = 20.0;

  bool altitude_manual_override = false;

  /**
   * Most recent altitude→layer band reported by UpdateAltitude().
   * Tracked continuously so OnManualLayerStep() can snapshot it.
   * -1 until the first altitude tick has arrived.
   */
  int last_known_band = -1;

  /**
   * The physical altitude band the aircraft was in when the pilot
   * last pressed ◀/▶ on the altitude row. Auto-switch only resumes
   * once the aircraft has flown into a *different* band — i.e. the
   * pilot's manual choice sticks until the altitude actually changes
   * to somewhere the auto logic would naturally re-evaluate.
   *
   * -1 means the snapshot is still pending (no altitude reading
   * arrived between class construction / reset and the manual step);
   * the next UpdateAltitude() tick binds it.
   */
  int manual_step_band = -1;

  /* --- Time auto-switch --- */
  std::vector<unsigned> loaded_times_utc; // available UTC hours (0-23)
  int current_time_pos = -1;

  bool time_manual_override = false;
  int last_switch_hour = -1; // track which hour we last auto-switched for

  /* --- Callbacks --- */
  SwitchCallback on_layer_switch;
  TimeSwitchCallback on_time_switch;

public:
  void SetEnabled(bool e) noexcept { enabled = e; }
  bool IsEnabled() const noexcept { return enabled; }

  /**
   * Set the available (loaded) altitude layers, sorted by altitude.
   * Recalculates midpoint thresholds.
   */
  void SetLoadedLayers(std::vector<LayerInfo> layers) noexcept;

  /**
   * Set the available forecast UTC hours.
   */
  void SetLoadedTimes(std::vector<unsigned> times) noexcept;

  /**
   * Set current positions (called after manual selection or initial load).
   */
  void SetCurrentLayerPos(int pos) noexcept;

  /**
   * Match @c current_layer_pos to a region layer index from
   * #SetLoadedLayers().
   */
  void SyncCurrentLayerIndex(unsigned layer_index) noexcept;

  void SetCurrentTimePos(int pos) noexcept;

  /**
   * Called when user manually steps the layer.
   * Pauses auto-altitude until next threshold crossing.
   */
  void OnManualLayerStep() noexcept;

  /**
   * Called when user manually steps the time.
   * Pauses auto-time until next threshold crossing.
   */
  void OnManualTimeStep() noexcept;

  /**
   * Clear altitude manual override and snap to the layer for @p altitude.
   */
  void ResumeAltitudeAuto(double altitude) noexcept;

  /** Clear time manual override (caller applies the auto time index). */
  void ResumeTimeAuto() noexcept;

  void SetLayerSwitchCallback(SwitchCallback cb) noexcept {
    on_layer_switch = std::move(cb);
  }

  void SetTimeSwitchCallback(TimeSwitchCallback cb) noexcept {
    on_time_switch = std::move(cb);
  }

  void SetAltitudeManualOverride(bool override) noexcept {
    altitude_manual_override = override;
  }

  void SetTimeManualOverride(bool override) noexcept {
    time_manual_override = override;
  }

  bool IsAltitudeManualOverride() const noexcept {
    return altitude_manual_override;
  }

  bool IsTimeManualOverride() const noexcept {
    return time_manual_override;
  }

  /**
   * Called periodically (~1 Hz) with current flight data.
   * @param gps_alt GPS altitude in meters (negative if unavailable)
   * @param baro_alt barometric altitude in meters (negative if unavailable)
   * @param utc_hour current UTC hour (0-23)
   * @param utc_minute current UTC minute (0-59)
   * @param now monotonic timestamp for hysteresis timing
   */
  void Update(double gps_alt, double baro_alt,
              unsigned utc_hour, unsigned utc_minute,
              TimeStamp now) noexcept;

  bool IsAltitudeAutoActive() const noexcept { return enabled && !altitude_manual_override; }
  bool IsTimeAutoActive() const noexcept { return enabled && !time_manual_override; }

private:
  void RecalculateThresholds() noexcept;
  void UpdateAltitude(double altitude, TimeStamp now) noexcept;
  void UpdateTime(unsigned utc_hour, unsigned utc_minute) noexcept;

  /**
   * Find which layer position the given altitude maps to.
   * Returns -1 if no layers loaded.
   */
  int FindLayerForAltitude(double altitude) const noexcept;
};
