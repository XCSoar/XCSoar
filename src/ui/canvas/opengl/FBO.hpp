/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_OPENGL_FBO_HPP
#define XCSOAR_OPENGL_FBO_HPP

#include "ui/opengl/SystemExt.hpp"

/**
 * Support for OpenGL framebuffer objects (GL_*_framebuffer_object).
 */
namespace FBO {

static constexpr GLenum RENDERBUFFER = GL_RENDERBUFFER;
static constexpr GLenum FRAMEBUFFER = GL_FRAMEBUFFER;
static constexpr GLenum COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0;
static constexpr GLenum DEPTH_ATTACHMENT = GL_DEPTH_ATTACHMENT;
static constexpr GLenum STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT;

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

} // namespace OpenGL

#endif
