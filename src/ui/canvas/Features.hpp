// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_MEMORY_CANVAS
#include "memory/Features.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#endif

#ifdef USE_GDI
#include "gdi/Features.hpp"
#endif

/**
 * Return true when the Canvas implements clipping against its
 * siblings and children.
 */
constexpr bool
HaveClipping()
{
#ifdef HAVE_CLIPPING
  return true;
#else
  return false;
#endif
}
