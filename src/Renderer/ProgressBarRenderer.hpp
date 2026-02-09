// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
class Color;

void
DrawSimpleProgressBar(Canvas &canvas, const PixelRect &r,
                      unsigned current_value,
                      unsigned min_value, unsigned max_value,
                      const Color *background_color = nullptr) noexcept;

void
DrawRoundProgressBar(Canvas &canvas, const PixelRect &r,
                     unsigned current_value,
                     unsigned min_value, unsigned max_value,
                     const Color *background_color = nullptr) noexcept;
