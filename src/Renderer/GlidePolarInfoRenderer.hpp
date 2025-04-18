// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
struct ChartLook;
class GlidePolar;

void
RenderGlidePolarInfo(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const GlidePolar &glide_polar);
