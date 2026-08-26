// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

#include "time/RoughTime.hpp"

struct PixelRect;
class Canvas;
struct ChartLook;
struct CrossSectionLook;
class FlightStatistics;
struct FlyingState;
struct NMEAInfo;
struct DerivedInfo;
class ProtectedTaskManager;
class TaskManager;

void
BarographCaption(char *buffer, size_t buffer_size,
                 const FlightStatistics &fs,
                 const FlyingState &flight,
                 RoughTimeDelta utc_offset) noexcept;

void
RenderBarographSpark(Canvas &canvas, const PixelRect rc,
                     const ChartLook &chart_look,
                     const CrossSectionLook &cross_section_look,
                     bool inverse,
                     const FlightStatistics &fs,
                     const NMEAInfo &nmea_info,
                     const DerivedInfo &derived_info,
                     const ProtectedTaskManager *_task);

void
RenderBarograph(Canvas &canvas, const PixelRect rc,
                const ChartLook &chart_look,
                const CrossSectionLook &cross_section_look,
                const FlightStatistics &fs,
                const NMEAInfo &nmea_info,
                const DerivedInfo &derived_info,
                const ProtectedTaskManager *_task);
