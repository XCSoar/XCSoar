// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct PixelRect;
class Canvas;
struct ChartLook;
class GlidePolar;
class FlightStatistics;

void
RenderVarioHistogram(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const FlightStatistics &fs,
                     const GlidePolar &glide_polar);
