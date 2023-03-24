// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * Specialisation of Dijkstra for WeGlide Out and Return rules
 */
class WeglideOR : public ContestDijkstra {
public:
  explicit WeglideOR(const Trace &_trace) noexcept;
};
