// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Bitmap.hpp"
#include "util/StaticArray.hxx"

class Path;
struct Waypoint;

/**
 * Load embedded waypoint pictures from a CUP/CUPX file into @p images.
 * @return number of bitmaps appended
 */
unsigned
LoadWaypointEmbeddedImages(StaticArray<Bitmap, 5> &images,
                           const Waypoint &waypoint,
                           Path source_path) noexcept;
