// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelPoint;
class Canvas;
struct TaskLook;
class Angle;
struct DerivedInfo;

namespace BestCruiseArrowRenderer
{
  void Draw(Canvas &canvas, const TaskLook &look, const Angle screen_angle,
            Angle best_cruise_angle, PixelPoint pos);

  void Draw(Canvas &canvas, const TaskLook &look, const Angle screen_angle,
            PixelPoint pos, const DerivedInfo &calculated);
}
