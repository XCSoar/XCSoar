// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

template <typename Duration>
class TimeSinceMidnight;

/**
 * This data type stores a time of day with minute granularity.
 */
using UnsignedMinutes = std::chrono::duration<uint16_t,std::chrono::minutes::period>;
using RoughTime = TimeSinceMidnight<UnsignedMinutes>;

