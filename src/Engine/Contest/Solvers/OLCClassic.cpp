// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OLCClassic.hpp"

OLCClassic::OLCClassic(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 6, 1000) {}
