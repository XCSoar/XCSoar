// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of Dijkstra for WeGlide Distance rules
 */
class WeglideDistance : public ContestDijkstra {
public:
  explicit WeglideDistance(const Trace &_trace) noexcept;
};
