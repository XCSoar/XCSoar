// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

struct PixelRect;
class Canvas;
struct ChartLook;
class FlightStatistics;
struct NMEAInfo;
struct DerivedInfo;
class TaskManager;
class GlidePolar;

void
TaskSpeedCaption(char *s_tmp, size_t buffer_size,
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
