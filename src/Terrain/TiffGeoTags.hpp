// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_LIBTIFF

#include <tiffio.h>

/* Stock libtiff may lack the GeoTIFF names; Kobo builds TIFF without
   libgeotiff.  Numbers are the GeoTIFF / GDAL tags. */
#ifndef TIFFTAG_GEOPIXELSCALE
#define TIFFTAG_GEOPIXELSCALE 33550
#endif
#ifndef TIFFTAG_GEOTIEPOINTS
#ifdef TIFFTAG_MODELTIEPOINTTAG
#define TIFFTAG_GEOTIEPOINTS TIFFTAG_MODELTIEPOINTTAG
#else
#define TIFFTAG_GEOTIEPOINTS 33922
#endif
#endif
#ifndef TIFFTAG_GDAL_NODATA
#define TIFFTAG_GDAL_NODATA 42113
#endif

void InstallCogTiffSupport() noexcept;

#endif
