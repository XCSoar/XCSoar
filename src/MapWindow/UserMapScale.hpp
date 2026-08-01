// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DisplayMode.hpp"

/**
 * Clamp a continuous user map scale (keyboard, mouse wheel, pinch,
 * Lua) to the same min/max used by InputEvents::sub_SetZoom().
 *
 * @param vmin  polar minimum speed (m/s); used for the cruise
 *              "2 minute" minimum scale
 */
[[nodiscard]] [[gnu::pure]]
double
ClampUserMapScale(double scale, DisplayMode mode, double vmin) noexcept;

/**
 * Clamp using the current display mode and polar from
 * #CommonInterface.
 */
[[nodiscard]]
double
ClampUserMapScale(double scale) noexcept;

/**
 * Turn auto-zoom off after a manual scale change.  No-op when
 * auto-zoom is already off, or while circling with circle-zoom.
 *
 * @return true if auto-zoom was disabled
 */
bool
DisableAutoZoomForManualScale() noexcept;
