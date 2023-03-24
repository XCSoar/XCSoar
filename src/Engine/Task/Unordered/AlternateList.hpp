// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AlternatePoint.hpp"

#include <vector>

/**
 * Vector of waypoints and solutions used to store candidates.
 */
class AlternateList : public std::vector<AlternatePoint> {
};
