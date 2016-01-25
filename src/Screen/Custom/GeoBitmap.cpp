/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/Bitmap.hpp"
#include "UncompressedImage.hpp"
#include "Geo/Quadrilateral.hpp"
#include "OS/Path.hpp"

#ifdef USE_GEOTIFF
#include "LibTiff.hpp"
#endif

#include <stdexcept>

#include <tchar.h>

#if !defined(USE_GEOTIFF) && GCC_CHECK_VERSION(4,9)
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#endif

GeoQuadrilateral
Bitmap::LoadGeoFile(Path path)
{
#ifdef USE_GEOTIFF
  if (path.MatchesExtension(_T(".tif")) ||
      path.MatchesExtension(_T(".tiff"))) {
    auto result = LoadGeoTiff(path);
    if (!Load(std::move(result.first)))
      throw std::runtime_error("Failed to use geo image file");

    assert(IsDefined());

    return result.second;
  }
#endif

  throw std::runtime_error("Unsupported geo image file");
}
