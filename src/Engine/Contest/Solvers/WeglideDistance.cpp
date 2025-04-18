// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideDistance.hpp"

WeglideDistance::WeglideDistance(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 6, 1000) {}
