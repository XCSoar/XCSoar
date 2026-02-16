// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct PixelRect;
class Canvas;
struct ChartLook;
class CuSonde;

void
RenderTemperatureChart(Canvas &canvas, const PixelRect rc,
                       const ChartLook &chart_look,
                       const CuSonde &cu_sonde);

void
TemperatureChartCaption(char *buffer, const CuSonde &cu_sonde);
