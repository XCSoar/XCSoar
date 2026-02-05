// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct PixelRect;
class Canvas;
struct ChartLook;
class GlidePolar;
class FlightStatistics;
struct NMEAInfo;
struct DerivedInfo;
class TaskManager;

void
ClimbChartCaption(char *buffer,
                  const FlightStatistics &fs);

void
RenderClimbChart(Canvas &canvas, const PixelRect rc,
                 const ChartLook &chart_look,
                 const FlightStatistics &fs,
                 const GlidePolar &glide_polar,
                 const NMEAInfo &nmea_info,
                 const DerivedInfo &derived_info,
                 const TaskManager &task);
