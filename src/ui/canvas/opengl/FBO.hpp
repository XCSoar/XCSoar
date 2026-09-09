// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/SystemExt.hpp"
#include "Dynamic.hpp"

#if (defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define GL_UNBIND_FRAMEBUFFER 1
#define GL_UNBIND_RENDERBUFFER 1
#else 
#define GL_UNBIND_FRAMEBUFFER 0
#define GL_UNBIND_RENDERBUFFER 0
#endif

/**
 * Support for OpenGL framebuffer objects (GL_*_framebuffer_object).
 */
namespace FBO {

static constexpr GLenum RENDERBUFFER = GL_RENDERBUFFER;
static constexpr GLenum FRAMEBUFFER = GL_FRAMEBUFFER;
static constexpr GLenum COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0;
static constexpr GLenum DEPTH_ATTACHMENT = GL_DEPTH_ATTACHMENT;
static constexpr GLenum STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT;

#ifdef HAVE_MULTISAMPLE_FBO

/* the read/draw framebuffer split used by the explicit multisample
   resolve; the GLES2 headers only ever spell these with a vendor
   suffix, but all of the spellings share the core enum value */
#ifdef GL_READ_FRAMEBUFFER
static constexpr GLenum READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER;
static constexpr GLenum DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER;
#elif defined(GL_READ_FRAMEBUFFER_NV)
static constexpr GLenum READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER_NV;
static constexpr GLenum DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER_NV;
#elif defined(GL_READ_FRAMEBUFFER_ANGLE)
static constexpr GLenum READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER_ANGLE;
static constexpr GLenum DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER_ANGLE;
#elif defined(GL_READ_FRAMEBUFFER_APPLE)
static constexpr GLenum READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER_APPLE;
static constexpr GLenum DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER_APPLE;
#else
#error No GL_READ_FRAMEBUFFER found
#endif

/* a sized colour format for the multisampled renderbuffer; the
   unsized GL_RGB the BufferCanvas texture uses is not a valid
   renderbuffer format */
#ifdef GL_RGB8
static constexpr GLenum RGB8 = GL_RGB8;
#elif defined(GL_RGB8_OES)
static constexpr GLenum RGB8 = GL_RGB8_OES;
#else
#error No GL_RGB8 found
#endif

#endif // HAVE_MULTISAMPLE_FBO

#ifdef GL_DEPTH_STENCIL
static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL;
#elif defined(GL_DEPTH_STENCIL_EXT)
static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL_EXT;
#elif defined(GL_DEPTH_STENCIL_NV)
static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL_NV;
#elif defined(GL_DEPTH_STENCIL_OES)
static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL_OES;
#else
#error No GL_DEPTH_STENCIL found
#endif

static inline void
BindRenderbuffer(GLenum target, GLuint renderbuffer) noexcept
{
  glBindRenderbuffer(target, renderbuffer);
}

static inline void
DeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) noexcept
{
  glDeleteRenderbuffers(n, renderbuffers);
}

static inline void
GenRenderbuffers(GLsizei n, GLuint *renderbuffers) noexcept
{
  glGenRenderbuffers(n, renderbuffers);
}

static inline void
RenderbufferStorage(GLenum target, GLenum internalformat,
                    GLsizei width, GLsizei height) noexcept
{
  glRenderbufferStorage(target, internalformat, width, height);
}

static inline void
BindFramebuffer(GLenum target, GLuint framebuffer) noexcept
{
  glBindFramebuffer(target, framebuffer);
}

static inline void
DeleteFramebuffers(GLsizei n, const GLuint *framebuffers) noexcept
{
  glDeleteFramebuffers(n, framebuffers);
}

static inline void
GenFramebuffers(GLsizei n, GLuint *framebuffers) noexcept
{
  glGenFramebuffers(n, framebuffers);
}

static inline void
FramebufferRenderbuffer(GLenum target, GLenum attachment,
                        GLenum renderbuffertarget,
                        GLuint renderbuffer) noexcept
{
  glFramebufferRenderbuffer(target, attachment,
                            renderbuffertarget, renderbuffer);
}

static inline void
FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                     GLuint texture, GLint level) noexcept
{
  glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

#ifdef HAVE_MULTISAMPLE_FBO

/**
 * Allocate multisampled renderbuffer storage.  The caller must have
 * checked OpenGL::fbo_antialiasing_samples first; @a samples must not
 * exceed it, and must match the sample count of every other
 * attachment of the framebuffer.
 */
static inline void
RenderbufferStorageMultisample(GLenum target, GLsizei samples,
                               GLenum internalformat,
                               GLsizei width, GLsizei height) noexcept
{
  GLExt::renderbuffer_storage_multisample(target, samples, internalformat,
                                          width, height);
}

/**
 * Attach a texture to a framebuffer as a multisampled colour buffer.
 * Rendering happens in an implicit multisample buffer which is
 * resolved into the texture when the framebuffer is unbound; there is
 * no explicit blit, and the texture itself is never multisampled.
 */
static inline void
FramebufferTexture2DMultisample(GLenum target, GLenum attachment,
                                GLenum textarget, GLuint texture,
                                GLint level, GLsizei samples) noexcept
{
  GLExt::framebuffer_texture_2d_multisample(target, attachment, textarget,
                                            texture, level, samples);
}

/**
 * Resolve a multisampled framebuffer bound to #READ_FRAMEBUFFER into
 * the single-sampled one bound to #DRAW_FRAMEBUFFER.  The caller must
 * have checked that OpenGL::fbo_antialiasing_mode is
 * FboAntialiasingMode::BLIT.
 */
static inline void
BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                GLbitfield mask, GLenum filter) noexcept
{
  GLExt::blit_framebuffer(srcX0, srcY0, srcX1, srcY1,
                          dstX0, dstY0, dstX1, dstY1, mask, filter);
}

#endif // HAVE_MULTISAMPLE_FBO

} // namespace OpenGL
