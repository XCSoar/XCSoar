/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_SCREEN_OPENGL_DYNAMIC_HPP
#define XCSOAR_SCREEN_OPENGL_DYNAMIC_HPP

#include "SystemExt.hpp"
#include "Features.hpp"

#if defined(GL_EXT_multi_draw_arrays) && defined(HAVE_GLES)
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

  static inline bool HaveMultiDrawElements() {
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
    return multi_draw_elements != nullptr;
#else
    return true;
#endif
  }

  template<typename... Args>
  static inline void MultiDrawElements(Args... args) {
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
    multi_draw_elements(args...);
#else
    glMultiDrawElementsEXT(args...);
#endif
  }
#endif /* GL_EXT_multi_draw_arrays */
};

#endif
