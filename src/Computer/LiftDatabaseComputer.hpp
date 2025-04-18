// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

struct MoreData;
struct CirclingInfo;
class LiftDatabase;
class TraceVariableHistory;

/**
 * This computer manages the LiftDatabase and
 * TraceHistory::CirclingAverage.
 *
 * Dependencies: #CirclingComputer.
 */
class LiftDatabaseComputer {
  bool last_circling;

  Angle last_heading;

  /**
   * Clear the attributes managed by this computer.
   */
  void Clear(LiftDatabase &lift_database,
             TraceVariableHistory &circling_average_trace);

public:
  /**
   * Reset this computer and clear the attributes.
   */
  void Reset(LiftDatabase &lift_database,
             TraceVariableHistory &circling_average_trace);

  void Compute(LiftDatabase &lift_database,
               TraceVariableHistory &circling_average_trace,
               const MoreData &basic, const CirclingInfo &circling_info);
};
