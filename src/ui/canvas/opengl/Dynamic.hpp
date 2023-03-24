// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/SystemExt.hpp"
#include "ui/opengl/Features.hpp"

#if defined(GL_EXT_multi_draw_arrays)
#define HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
#endif

namespace GLExt {

#ifdef HAVE_DYNAMIC_MAPBUFFER
extern PFNGLMAPBUFFEROESPROC map_buffer;
extern PFNGLUNMAPBUFFEROESPROC unmap_buffer;
#endif

#ifdef GL_EXT_multi_draw_arrays
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
extern PFNGLMULTIDRAWARRAYSEXTPROC multi_draw_arrays;
extern PFNGLMULTIDRAWELEMENTSEXTPROC multi_draw_elements;
#endif

static inline bool HaveMultiDrawElements() noexcept {
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
  return multi_draw_elements != nullptr;
#else
  return true;
#endif
}

template<typename... Args>
static inline void MultiDrawElements(Args... args) noexcept {
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
  multi_draw_elements(args...);
#else
  glMultiDrawElementsEXT(args...);
#endif
}
#endif /* GL_EXT_multi_draw_arrays */

} // namespace GLExt
