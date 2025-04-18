// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/OverwritingRingBuffer.hpp"
#include "NMEA/Validity.hpp"

#include <type_traits>

class TraceVariableHistory: public TrivialOverwritingRingBuffer<double, 30> {};

struct MoreData;

class TraceHistory {
public:
  TraceVariableHistory BruttoVario;
  TraceVariableHistory NettoVario;
  TraceVariableHistory CirclingAverage;

  /**
   * Just time stamps describing when the fields above were last
   * modified.
   */
  Validity vario_available, circling_available;

  void append(const MoreData &basic) noexcept;
  void clear() noexcept;
};

static_assert(std::is_trivial<TraceHistory>::value, "type is not trivial");
