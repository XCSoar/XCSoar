// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/SystemExt.hpp"
#include "ui/opengl/Features.hpp"

#if defined(GL_EXT_multi_draw_arrays)
#define HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
#endif

/**
 * Can this build render a framebuffer object with multisampling?
 * Both mechanisms need tokens and typedefs from the extension
 * headers, and the code paths are written as a pair, so they are
 * gated together.  Whether the running implementation actually offers
 * one of them is OpenGL::fbo_antialiasing_mode.
 */
#if defined(GL_EXT_multisampled_render_to_texture) && \
  defined(GL_NV_framebuffer_blit)
#define HAVE_MULTISAMPLE_FBO
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

#ifdef HAVE_MULTISAMPLE_FBO
/**
 * Allocate multisampled renderbuffer storage.  Both multisample
 * mechanisms need this, and both spell the entry point
 * "glRenderbufferStorageMultisample" with an assortment of vendor
 * suffixes; whichever one resolves is stored here.
 */
inline PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC
  renderbuffer_storage_multisample;

/**
 * Attach a texture as a multisampled colour buffer, resolved
 * implicitly when the framebuffer is unbound
 * (GL_EXT_multisampled_render_to_texture).  nullptr if the
 * implementation does not offer implicit resolve; tiled GPUs do,
 * desktop GPUs generally do not.
 */
inline PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC
  framebuffer_texture_2d_multisample;

/**
 * Resolve a multisampled framebuffer explicitly by blitting it into a
 * single-sampled one.  nullptr if the implementation cannot do that;
 * the NV typedef is used because it is the spelling the GLES2 headers
 * always provide, but the entry point loaded may be any of the core,
 * EXT, NV or ANGLE variants, which share this signature.
 */
inline PFNGLBLITFRAMEBUFFERNVPROC blit_framebuffer;
#endif // HAVE_MULTISAMPLE_FBO

} // namespace GLExt
