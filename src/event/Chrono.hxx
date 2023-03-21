// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <chrono>

namespace Event {

/**
 * The clock used by classes #EventLoop, #CoarseTimerEvent and #FineTimerEvent.
 */
using Clock = std::chrono::steady_clock;

using Duration = Clock::duration;
using TimePoint = Clock::time_point;

} // namespace Event
