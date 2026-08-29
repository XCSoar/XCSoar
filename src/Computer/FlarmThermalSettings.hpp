// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

/** Initial tunable values for the FLARM thermal detector. */
namespace FlarmThermalConstants {
static constexpr FloatDuration CONTRIBUTOR_TIMEOUT{10};
static constexpr FloatDuration GROUPING_TIME_GAP{120};
static constexpr FloatDuration EXIT_TURN_WINDOW{5};

static constexpr double ENTER_CLIMB_THRESHOLD = 0.5;
static constexpr double EXIT_CLIMB_THRESHOLD = 0.3;
static constexpr double MAX_DRIFT_CORRECTED_RADIUS = 500;
static constexpr double GROUPING_RADIUS = 500;
static constexpr double MIN_ACCUMULATED_TURN = 270;
static constexpr double MIN_RECENT_TURN_RATE = 4;
static constexpr double MIN_CURRENT_TURN_RATE = 3;

/** Enough for the complete window after rate-aware sample coalescing. */
static constexpr unsigned MAX_SAMPLE_COUNT = 128;
}
