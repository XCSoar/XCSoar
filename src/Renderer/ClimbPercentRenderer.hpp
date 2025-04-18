// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
struct CirclingInfo;
struct ClimbPercentLook;

class ClimbPercentRenderer {
  const ClimbPercentLook &look;

public:
  ClimbPercentRenderer(const ClimbPercentLook &_look):look(_look) {}

  void Draw(const CirclingInfo &stats, Canvas &canvas,
            const PixelRect &rc, bool inverse);
};
