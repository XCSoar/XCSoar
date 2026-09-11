// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenTime.hpp"

#include <chrono>

class OperationEnvironment;
class Path;

/**
 * Takeoff/landing metadata extracted from an IGC file.
 */
struct IGCFlightTimes {
  bool has_valid_fixes{false};
  bool takeoff_detected{false};
  bool landing_detected{false};
  BrokenTime takeoff;
  BrokenTime landing;
  std::chrono::seconds duration{};
};

/**
 * Detect the first takeoff and final landing in an IGC file.
 *
 * This uses the same #FlyingComputer state machine as XCSoar.  Imported
 * files deliberately use a fixed threshold instead of the active plane
 * polar.  If detection is inconclusive, the corresponding first or last
 * valid fix is returned with the matching *_detected flag cleared.
 *
 * Throws on file errors and throws #OperationCancelled if @p operation is
 * cancelled.
 */
IGCFlightTimes
DetectIGCFlightTimes(Path path, OperationEnvironment *operation=nullptr);
