// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct FAITriangleSettings;

static constexpr unsigned FAI_TRIANGLE_SECTOR_MAX = 8 * 3 * 10;

/**
 * @return a pointer after the last generated item
 */
GeoPoint *
GenerateFAITriangleArea(GeoPoint *dest,
                        const GeoPoint &pt1, const GeoPoint &pt2,
                        bool reverse,
                        const FAITriangleSettings &settings);
