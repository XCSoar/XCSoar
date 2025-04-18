// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
class Angle;
struct NextArrowLook;
struct PixelRect;

class NextArrowRenderer {
  const NextArrowLook &look;

public:
  NextArrowRenderer(const NextArrowLook &_look):look(_look) {}

  void DrawArrow(Canvas &canvas, const PixelRect &rc, Angle angle);
};
