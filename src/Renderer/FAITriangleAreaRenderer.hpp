// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
class Canvas;
class WindowProjection;
struct FAITriangleSettings;

void
RenderFAISector(Canvas &canvas, const WindowProjection &projection,
                const GeoPoint &pt1, const GeoPoint &pt2,
                bool reverse, const FAITriangleSettings &settings) noexcept;
