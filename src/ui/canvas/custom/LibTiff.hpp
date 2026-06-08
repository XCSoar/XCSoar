// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <utility>

class Path;
class UncompressedImage;
struct GeoReferencedGrid;

/**
 * Load a TIFF file.  Throws a std::runtime_error on error.
 */
UncompressedImage
LoadTiff(Path path);

/**
 * Load a GeoTIFF file.  Throws a std::runtime_error on error.
 *
 * @return the image and its georeference grid
 */
std::pair<UncompressedImage, GeoReferencedGrid>
LoadGeoTiff(Path path);
