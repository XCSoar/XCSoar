// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

class Canvas;
class Angle;
struct PixelPoint;
struct PixelRect;
struct WindArrowLook;
struct SpeedVector;
struct DerivedInfo;
struct MapSettings;
enum class WindArrowStyle : uint8_t;

class WindArrowRenderer {
  const WindArrowLook &look;

public:
  explicit WindArrowRenderer(const WindArrowLook &_look) noexcept
    :look(_look) {}

  void Draw(Canvas &canvas, Angle screen_angle, SpeedVector wind,
            PixelPoint pos, const PixelRect &rc, WindArrowStyle arrow_style) noexcept;

  void Draw(Canvas &canvas, Angle screen_angle, PixelPoint pos,
            const PixelRect &rc, const DerivedInfo &calculated,
            const MapSettings &settings) noexcept;

  void DrawArrow(Canvas &canvas, PixelPoint pos, Angle angle,
                 unsigned width, unsigned length, unsigned tail_length,
                 WindArrowStyle arrow_style,
                 int offset,
                 unsigned scale) noexcept;
};
