// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"

struct PixelRect;
class Canvas;
class Angle;

/**
 * This class helps with drawing a "radar", i.e. a gauge which shows
 * centered circles.
 */
class RadarRenderer {
  PixelPoint center;

  /**
   * The minimum distance between the window boundary and the biggest
   * circle in pixels.
   */
  unsigned h_padding, v_padding;

  /**
   * The radius of the biggest circle in pixels.
   */
  unsigned view_radius;

public:
  constexpr RadarRenderer(unsigned _h_padding, unsigned _v_padding) noexcept
    :h_padding(_h_padding), v_padding(_v_padding) {}

  explicit constexpr RadarRenderer(unsigned _padding) noexcept
    :RadarRenderer(_padding, _padding) {}

  constexpr const PixelPoint &GetCenter() const noexcept {
    return center;
  }

  constexpr unsigned GetRadius() const noexcept {
    return view_radius;
  }

  void UpdateLayout(const PixelRect &rc) noexcept;

  constexpr PixelPoint At(int x, int y) const noexcept {
    return center.At(x, y);
  }

  [[gnu::pure]]
  PixelPoint At(Angle angle, unsigned radius) const noexcept;

  void DrawCircle(Canvas &canvas, unsigned circle_radius) const noexcept;
};
