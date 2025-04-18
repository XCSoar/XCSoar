// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL
#include "opengl/Canvas.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "memory/Canvas.hpp"
#elif defined(USE_GDI)
#include "gdi/Canvas.hpp"
#else
#error No Canvas implementation
#endif
