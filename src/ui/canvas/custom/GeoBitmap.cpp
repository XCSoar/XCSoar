// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "UncompressedImage.hpp"
#include "Geo/Quadrilateral.hpp"
#include "system/Path.hpp"

#ifdef USE_GEOTIFF
#include "LibTiff.hpp"
#endif

#include <stdexcept>

#ifdef USE_GEOTIFF
GeoQuadrilateral
Bitmap::LoadGeoFile(Path path)
{
  if (path.EndsWithIgnoreCase(".tif") ||
      path.EndsWithIgnoreCase(".tiff")) {
    auto result = LoadGeoTiff(path);
    if (!Load(std::move(result.first)))
      throw std::runtime_error("Failed to use geo image file");

    assert(IsDefined());

    return result.second;
  }

  throw std::runtime_error("Unsupported geo image file");
}
#else
[[noreturn]] GeoQuadrilateral
Bitmap::LoadGeoFile([[maybe_unused]] Path)
{
  throw std::runtime_error("Unsupported geo image file");
}
#endif
