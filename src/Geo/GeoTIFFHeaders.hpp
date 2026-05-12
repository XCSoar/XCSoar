// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#if defined(__has_include)

#if __has_include(<geotiff.h>)
#include <geo_normalize.h>
#include <geotiff.h>
#include <geotiffio.h>
#include <geovalues.h>
#include <xtiffio.h>
#elif __has_include(<geotiff/geotiff.h>)
#include <geotiff/geo_normalize.h>
#include <geotiff/geotiff.h>
#include <geotiff/geotiffio.h>
#include <geotiff/geovalues.h>
#include <geotiff/xtiffio.h>
#else
#error "libgeotiff headers not found"
#endif

#else

#include <geo_normalize.h>
#include <geotiff.h>
#include <geotiffio.h>
#include <geovalues.h>
#include <xtiffio.h>

#endif