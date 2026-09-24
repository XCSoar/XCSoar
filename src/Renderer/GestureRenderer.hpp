// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"

#include <span>

class Canvas;
struct GestureLook;

namespace GestureRenderer {

/**
 * Draw a gesture trail.
 *
 * @param points the recorded pointer locations
 * @param valid draw the trail in the "recognised gesture" colour
 */
void
Draw(Canvas &canvas, const GestureLook &look,
     std::span<const PixelPoint> points, bool valid) noexcept;

} // namespace GestureRenderer
