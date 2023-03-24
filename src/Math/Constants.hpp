// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef __MSVC__
# include <corecrt_math_defines.h>
#endif

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_2PI
#define M_2PI 6.28318530718
#endif
#ifndef M_PI_2
#define M_PI_2 1.5707963268
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

static constexpr double DEG_TO_RAD = .0174532925199432958;
static constexpr double RAD_TO_DEG = 57.2957795131;
