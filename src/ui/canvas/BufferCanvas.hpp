// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL
#include "opengl/BufferCanvas.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "memory/BufferCanvas.hpp"
#else /* GDI */
#include "gdi/BufferCanvas.hpp"
#endif
