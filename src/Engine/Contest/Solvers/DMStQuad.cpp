// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DMStQuad.hpp"

DMStQuad::DMStQuad(const Trace &_trace) noexcept
  :ContestDijkstra(_trace, true, 4, 1000) {}
