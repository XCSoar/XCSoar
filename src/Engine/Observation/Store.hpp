// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "util/TrivialArray.hxx"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

/**
 * Which feed produced a thermal or a wave line.
 * OWN is XCSoar's own locator / wave computer.
 */
enum class ObservationOrigin : uint8_t {
  OWN,
  CLOUD,
  SKYLINES,
  TIM,
  COUNT,
};

/**
 * One thermal mark. @c received is when XCSoar learned the row, so
 * feeds that do not share a clock can still be compared.
 */
struct ObservedThermal {
  ObservationOrigin origin = ObservationOrigin::OWN;
  GeoPoint location;
  double ground_height = 0;
  double lift_rate = 0;
  std::chrono::steady_clock::time_point received{};
};

/** One wave line. */
struct ObservedWave {
  ObservationOrigin origin = ObservationOrigin::OWN;
  GeoPoint a, b;
  std::chrono::steady_clock::time_point received{};
};

/** Rows kept per origin. Cloud and SkyLines already drop past this. */
inline constexpr unsigned OBSERVATION_CAP = 64;

inline constexpr std::size_t OBSERVATION_ROW_CAP =
  OBSERVATION_CAP * unsigned(ObservationOrigin::COUNT);

using ThermalRows = TrivialArray<ObservedThermal, OBSERVATION_ROW_CAP>;
using WaveRows = TrivialArray<ObservedWave, OBSERVATION_ROW_CAP>;

/**
 * shortcut: icons closer than this are one thermal and the newer
 * @c received wins; widen it when FLARM thermals (#832) land farther
 * from the XCSoar mark.
 */
inline constexpr double THERMAL_DUPLICATE_RANGE_M = 1000;

/** Same own-ship source across republishes, so its received time stays. */
inline constexpr double OWN_THERMAL_MATCH_RANGE_M = 50;

constexpr bool
OriginShown(ObservationOrigin origin,
            bool show_cloud, bool show_tim) noexcept
{
  switch (origin) {
  case ObservationOrigin::CLOUD:
    return show_cloud;
  case ObservationOrigin::TIM:
    return show_tim;
  case ObservationOrigin::OWN:
  case ObservationOrigin::SKYLINES:
    return true;
  case ObservationOrigin::COUNT:
    break;
  }
  return false;
}

/**
 * Thermals and wave lines from several feeds. Each feed replaces or
 * appends only its own rows.
 */
class ObservationStore {
  ThermalRows thermals;
  WaveRows waves;

public:
  ObservationStore() noexcept {
    thermals.clear();
    waves.clear();
  }

  void ReplaceThermals(ObservationOrigin origin,
                       std::span<const ObservedThermal> rows) noexcept;
  void AppendThermal(const ObservedThermal &row) noexcept;

  void ReplaceWaves(ObservationOrigin origin,
                    std::span<const ObservedWave> rows) noexcept;
  void AppendWave(const ObservedWave &row) noexcept;

  std::span<const ObservedThermal> Thermals() const noexcept {
    return thermals;
  }

  std::span<const ObservedWave> Waves() const noexcept {
    return waves;
  }
};

/**
 * Keep one row when two drawn positions are within @p range_m.
 * The newer @c received time wins.
 */
void
SelectThermals(std::span<const ObservedThermal> in,
               ThermalRows &out, double range_m) noexcept;
