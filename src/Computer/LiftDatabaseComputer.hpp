// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include <deque>

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
  std::deque<double> time_buffer;
  std::deque<Angle> heading_delta_buffer;
  std::deque<double> brutto_vario_buffer;
  std::deque<Angle> heading_buffer;
  std::deque<Angle> smoothed_turn_rate_buffer;

  Angle last_heading;

  /**
   * Clear the attributes managed by this computer.
   */
  void Clear(LiftDatabase &lift_database,
             TraceVariableHistory &circling_average_trace);

  void Clear(TraceVariableHistory &TurnAverage);

public:
  /**
   * Reset this computer and clear the attributes.
   */
  void Reset(LiftDatabase &lift_database,
             TraceVariableHistory &circling_average_trace,
             TraceVariableHistory &TurnAverage);

  void Compute(LiftDatabase &lift_database,
               TraceVariableHistory &circling_average_trace,
               const MoreData &basic, const CirclingInfo &circling_info);

  void
  Compute(int &turn_time,
          TraceVariableHistory &TurnAverage,
          const MoreData &basic,
          const CirclingInfo &circling_info);

private:
  void
  UpdateLastTurn(const Angle &current_heading,
                 double current_time,
                 double current_brutto_vario,
                 const Angle &smoothed_turn_rate);
};
