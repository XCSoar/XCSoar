// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelPoint;
struct PixelRect;
struct MapLook;
class Canvas;
class Angle;

class CompassRenderer {
  const MapLook &look;

public:
  CompassRenderer(const MapLook &_look) noexcept:look(_look) {}

  /**
   * The position at which Draw(Canvas &, Angle, PixelRect) draws the
   * compass within the given map rectangle; also used for tap
   * hit-testing.
   */
  [[gnu::pure]]
  static PixelPoint GetPosition(PixelRect rc) noexcept;

  void Draw(Canvas &canvas, Angle screen_angle, PixelPoint pos) noexcept;
  void Draw(Canvas &canvas, Angle screen_angle, PixelRect rc) noexcept;
};
