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

  void Draw(Canvas &canvas, Angle screen_angle, PixelPoint pos) noexcept;
  void Draw(Canvas &canvas, Angle screen_angle, PixelRect rc) noexcept;

  /**
   * How far the arrow centre sits in from the top-right of the
   * content rectangle.
   */
  [[gnu::const]]
  static unsigned GetCenterInset() noexcept;

  /**
   * How far the arrow glyph reaches past its centre.
   */
  [[gnu::const]]
  static unsigned GetGlyphRadius() noexcept;

  /**
   * Height of the slot the pan readout leaves free below the
   * content-rect top while the arrow is drawn.
   */
  [[gnu::const]]
  static unsigned GetSlotHeight() noexcept {
    return GetCenterInset() + GetGlyphRadius();
  }
};
