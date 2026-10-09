// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/RangeEstimate.hpp"
#include "system/Path.hpp"
#include "thread/Mutex.hxx"
#include "time/PeriodClock.hpp"

struct NMEAInfo;

/**
 * Collects a #FlarmRangeEstimate across flights and keeps it in a
 * file.  It runs in the calculation thread; the user interface reads
 * and resets it through the methods below, which lock.
 */
class FlarmRangeComputer {
  mutable Mutex mutex;

  /** protected by #mutex */
  FlarmRangeEstimate estimate;

  FlarmRangeEstimator estimator;

  AllocatedPath path;

  /** when the estimate was saved last while flying */
  PeriodClock save_clock;

  bool was_flying = false;

  /** has #estimate changed since it was saved? (protected by #mutex) */
  bool modified = false;

public:
  /**
   * Load the estimate from @p _path, which is also where it is saved.
   */
  void Load(Path _path) noexcept;

  /**
   * Write the estimate to its file, if it has changed.
   */
  void Save() noexcept;

  /**
   * Count the FLARM traffic of this update.  Saves on landing, and
   * every 10 minutes in flight.
   */
  void Process(const NMEAInfo &basic, bool flying) noexcept;

  [[gnu::pure]]
  FlarmRangeEstimate GetEstimate() const noexcept {
    const std::lock_guard lock{mutex};
    return estimate;
  }

  /**
   * Clear the estimate, e.g. after a change to the antenna, and save
   * that.
   */
  void Reset() noexcept;
};
