// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
struct ChartLook;
class FlightStatistics;
struct NMEAInfo;
struct DerivedInfo;
class TaskManager;
class GlidePolar;

#include <tchar.h>

void
TaskSpeedCaption(char *sTmp,
                 const FlightStatistics &fs,
                 const GlidePolar &glide_polar);

void
RenderSpeed(Canvas &canvas, const PixelRect rc,
            const ChartLook &chart_look,
            const FlightStatistics &fs,
            const NMEAInfo &nmea_info,
            const DerivedInfo &derived_info,
            const TaskManager &task,
            const GlidePolar &glide_polar);
