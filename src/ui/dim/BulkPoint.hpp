// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL
#include "opengl/BulkPoint.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "memory/BulkPoint.hpp"
#elif defined(USE_GDI)
#include "gdi/BulkPoint.hpp"
#else
#error No Point implementation
#endif
