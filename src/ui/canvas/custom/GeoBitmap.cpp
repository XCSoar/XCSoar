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

#include <tchar.h>

#if !defined(USE_GEOTIFF) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#endif

GeoQuadrilateral
Bitmap::LoadGeoFile([[maybe_unused]] Path path)
{
#ifdef USE_GEOTIFF
  if (path.EndsWithIgnoreCase(_T(".tif")) ||
      path.EndsWithIgnoreCase(_T(".tiff"))) {
    auto result = LoadGeoTiff(path);
    if (!Load(std::move(result.first)))
      throw std::runtime_error("Failed to use geo image file");

    assert(IsDefined());

    return result.second;
  }
#endif

  throw std::runtime_error("Unsupported geo image file");
}
