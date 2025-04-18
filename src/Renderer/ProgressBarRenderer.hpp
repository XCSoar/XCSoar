// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;

void
DrawSimpleProgressBar(Canvas &canvas, const PixelRect &r,
                      unsigned current_value,
                      unsigned min_value, unsigned max_value) noexcept;

void
DrawRoundProgressBar(Canvas &canvas, const PixelRect &r,
                     unsigned current_value,
                     unsigned min_value, unsigned max_value) noexcept;
