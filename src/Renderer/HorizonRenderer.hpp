// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
struct HorizonLook;
struct AttitudeState;
class Canvas;

namespace HorizonRenderer
{
  void Draw(Canvas &canvas, const PixelRect &rc,
            const HorizonLook &look,
            const AttitudeState &attitude);
}
