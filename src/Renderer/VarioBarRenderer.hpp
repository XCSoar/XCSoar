// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct MoreData;
struct PixelRect;
class Canvas;
struct Color;
struct DerivedInfo;
struct VarioBarLook;
class GlidePolar;

class VarioBarRenderer {
  const VarioBarLook &look;

public:
  VarioBarRenderer(const VarioBarLook &_look)
    :look(_look) {}

  const VarioBarLook &GetLook() const {
    return look;
  }

  void Draw(Canvas &canvas, const PixelRect &rc,
            const MoreData &basic,
            const DerivedInfo &calculated,
            const GlidePolar &glide_polar,
            const bool vario_bar_avg_enabled) const;

  /** Speed-to-fly push/pull bar for the panel variometer margin strip. */
  void DrawSpeedToFly(Canvas &canvas, const PixelRect &rc,
                      const MoreData &basic,
                      const DerivedInfo &calculated,
                      unsigned max_bar_half_length,
                      Color pull_color, Color push_color) const;
};
