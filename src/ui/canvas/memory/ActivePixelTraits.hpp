// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PixelTraits.hpp"

#ifdef GREYSCALE
using ActivePixelTraits = GreyscalePixelTraits;
#else
using ActivePixelTraits = BGRAPixelTraits;
#endif
