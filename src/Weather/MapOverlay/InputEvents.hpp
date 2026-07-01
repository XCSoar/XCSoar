// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace WeatherMapOverlay {

/**
 * Dispatch a weather overlay input event to the active map-bottom
 * controls (time, altitude/level, auto/manual).
 */
void HandleInputEvent(const char *misc) noexcept;

} // namespace WeatherMapOverlay
