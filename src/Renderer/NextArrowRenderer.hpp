// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
class Angle;
struct WindArrowLook;
struct PixelRect;

class NextArrowRenderer {
  const WindArrowLook &look;

public:
  NextArrowRenderer(const WindArrowLook &_look):look(_look) {}

  void DrawArrow(Canvas &canvas, const PixelRect &rc, Angle angle);
};
