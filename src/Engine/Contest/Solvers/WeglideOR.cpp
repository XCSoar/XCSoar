// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeglideOR.hpp"

WeglideOR::WeglideOR(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 2, 1000) {}
