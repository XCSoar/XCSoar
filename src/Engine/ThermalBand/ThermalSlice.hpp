// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/FloatDuration.hxx"

#include <type_traits>

/*
  Structure to maintain information about average climb rates through a particular
  altitude.  Handles both time-averaged and encounter-averaged climb rates.

 */
struct ThermalSlice {
  // climb rate averaged by time [m/s]
  double w_t;

  // climb rate averaged by number of encounters [m/s]
  double w_n;

  // number of encounters (passages through this slice).  This is real valued as
  // fractional values are calculated during merging, and also to track the proportion
  // of travel to the next (as yet unvisited) slice.
  double n;

  // Time accumulated climbing up to this slice. [s]
  FloatDuration time;

  // Time to arrive at next slice [s]
  FloatDuration dt;

  // Set the time of passage through this slice.
  void SetSample(FloatDuration _time) noexcept {
    time = _time;
    n = 1.;
  }

  // combine this slice with the other
  void Merge(const ThermalSlice &o) noexcept;

  // update climb statistics other slice's time and height difference
  void Update(const ThermalSlice &o, double dh) noexcept;

  void Reset() noexcept {
    w_n = 0;
    w_t = 0;
    n = 0;
    time = {};
    dt = {};
  }

  // whether this item has data
  bool Occupied() const noexcept {
    return n > 0;
  }
};

static_assert(std::is_trivial<ThermalSlice>::value, "type is not trivial");
