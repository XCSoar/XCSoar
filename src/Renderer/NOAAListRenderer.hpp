// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/NOAAStore.hpp"

struct PixelRect;
class Canvas;
class TwoTextRowsRenderer;
struct NOAALook;

namespace NOAAListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const NOAAStore::Item &station,
            const TwoTextRowsRenderer &row_renderer);

  void Draw(Canvas &canvas, const PixelRect rc, const NOAAStore::Item &station,
            const NOAALook &look,
            const TwoTextRowsRenderer &row_renderer);
}
