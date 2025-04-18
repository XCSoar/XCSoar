// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL
#include "opengl/BufferCanvas.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "VirtualCanvas.hpp"
using BufferCanvas = VirtualCanvas;
#else /* GDI */
#include "gdi/BufferCanvas.hpp"
#endif
