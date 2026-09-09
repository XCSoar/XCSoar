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
inline PFNGLMAPBUFFEROESPROC map_buffer;
inline PFNGLUNMAPBUFFEROESPROC unmap_buffer;
#endif

#ifdef GL_EXT_multi_draw_arrays
#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
inline PFNGLMULTIDRAWARRAYSEXTPROC multi_draw_arrays;
inline PFNGLMULTIDRAWELEMENTSEXTPROC multi_draw_elements;
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

#ifdef GL_EXT_discard_framebuffer
inline PFNGLDISCARDFRAMEBUFFEREXTPROC discard_framebuffer;
#endif // GL_EXT_discard_framebuffer

#ifdef GL_EXT_multisampled_render_to_texture
/**
 * Render a framebuffer object with multisampling, resolved implicitly
 * when the framebuffer is unbound.  Both pointers are loaded together
 * and are either both valid or both nullptr.
 */
inline PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC
  framebuffer_texture_2d_multisample;
inline PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC
  renderbuffer_storage_multisample;
#endif // GL_EXT_multisampled_render_to_texture

} // namespace GLExt
