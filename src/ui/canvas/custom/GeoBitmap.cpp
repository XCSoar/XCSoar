// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "UncompressedImage.hpp"
#include "Geo/Quadrilateral.hpp"
#include "Geo/ReferencedGrid.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"

#ifdef USE_GEOTIFF
#include "LibTiff.hpp"
#endif

#include <stdexcept>

#ifdef USE_GEOTIFF
/* Smallest useful GeoTIFF is far larger; skip obvious truncation before LibTiff. */
static constexpr unsigned kMinTiffGeoFileSize = 100;

GeoReferencedGrid
Bitmap::LoadGeoFile(Path path)
{
  if (path.EndsWithIgnoreCase(".tif") ||
      path.EndsWithIgnoreCase(".tiff")) {
    if (File::GetSize(path) < kMinTiffGeoFileSize)
      throw std::runtime_error("Geo image file is too small");

    auto result = LoadGeoTiff(path);
    if (!Load(std::move(result.first)))
      throw std::runtime_error("Failed to use geo image file");

    assert(IsDefined());

    return result.second;
  }

  throw std::runtime_error("Unsupported geo image file");
}
#else
[[noreturn]] GeoReferencedGrid
Bitmap::LoadGeoFile([[maybe_unused]] Path)
{
  throw std::runtime_error("Unsupported geo image file");
}
#endif
