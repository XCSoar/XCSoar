// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Observation/Store.hpp"
#include "thread/Mutex.hxx"

/**
 * The map's thermal and wave lists. Writers replace or append one
 * origin under the mutex. The draw thread copies under the mutex.
 */
class ObservationDatabase {
  mutable Mutex mutex;
  ObservationStore store;

public:
  void ReplaceThermals(ObservationOrigin origin,
                       std::span<const ObservedThermal> rows) noexcept {
    const std::lock_guard lock{mutex};
    store.ReplaceThermals(origin, rows);
  }

  void AppendThermal(const ObservedThermal &row) noexcept {
    const std::lock_guard lock{mutex};
    store.AppendThermal(row);
  }

  void ReplaceWaves(ObservationOrigin origin,
                    std::span<const ObservedWave> rows) noexcept {
    const std::lock_guard lock{mutex};
    store.ReplaceWaves(origin, rows);
  }

  void AppendWave(const ObservedWave &row) noexcept {
    const std::lock_guard lock{mutex};
    store.AppendWave(row);
  }

  void CopyThermals(ThermalRows &out) const noexcept {
    const std::lock_guard lock{mutex};
    out.clear();
    for (const auto &row : store.Thermals()) {
      if (out.full())
        break;
      out.append(row);
    }
  }

  void CopyWaves(WaveRows &out) const noexcept {
    const std::lock_guard lock{mutex};
    out.clear();
    for (const auto &row : store.Waves()) {
      if (out.full())
        break;
      out.append(row);
    }
  }
};
