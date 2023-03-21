// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dynamic.hpp"

namespace GLExt {

#ifdef HAVE_DYNAMIC_MAPBUFFER
PFNGLMAPBUFFEROESPROC map_buffer;
PFNGLUNMAPBUFFEROESPROC unmap_buffer;
#endif

#if defined(GL_EXT_multi_draw_arrays) && defined(HAVE_DYNAMIC_MULTI_DRAW_ARRAYS)
PFNGLMULTIDRAWARRAYSEXTPROC multi_draw_arrays;
PFNGLMULTIDRAWELEMENTSEXTPROC multi_draw_elements;
#endif

} // namespace GLExt
