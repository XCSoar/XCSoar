// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <chrono>

/**
 * Simple map stopwatch shown as a centred pill overlay.  Controlled
 * from Status → Times; tap start/stop, long-press reset.
 */
namespace MapTimer {

[[gnu::pure]]
bool IsVisible() noexcept;

[[gnu::pure]]
bool IsRunning() noexcept;

void SetVisible(bool visible) noexcept;

/** Toggle visibility; does not reset elapsed time. */
void ToggleVisible() noexcept;

/** Start if stopped, stop if running. */
void ToggleRunning() noexcept;

/** Stop and clear elapsed time. */
void Reset() noexcept;

/** Elapsed time for display (includes current run if running). */
[[gnu::pure]]
std::chrono::seconds GetElapsed() noexcept;

} // namespace MapTimer
