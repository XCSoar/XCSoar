// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of OLC Dijkstra for OLC Classic rules
 */
class OLCClassic : public ContestDijkstra {
public:
  explicit OLCClassic(const Trace &_trace) noexcept;
};
