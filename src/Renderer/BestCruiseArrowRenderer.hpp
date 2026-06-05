// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct BulkPixelPoint;
struct PixelPoint;
class Canvas;
struct TaskLook;
class Angle;
struct DerivedInfo;

namespace BestCruiseArrowRenderer
{
  static constexpr unsigned arrow_size = 7;
  static constexpr int center_y = -55;

  [[nodiscard]] unsigned GetScale() noexcept;

  void Build(BulkPixelPoint *dest, int y_offset=0) noexcept;

  [[nodiscard]] int YOffsetForRadius(unsigned radius_from_center,
                                     int scale) noexcept;

  void Draw(Canvas &canvas, const TaskLook &look, const Angle screen_angle,
            Angle best_cruise_angle, PixelPoint pos);

  void Draw(Canvas &canvas, const TaskLook &look, const Angle screen_angle,
            PixelPoint pos, const DerivedInfo &calculated);
}
