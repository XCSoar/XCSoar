/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Features.hpp"
#include "SystemExt.hpp"
#include "Compiler.h"

/**
 * Support for OpenGL framebuffer objects (GL_*_framebuffer_object).
 */
namespace FBO {
#ifdef HAVE_GLES
  /* on GLES, the functions will be looked up dynamically */

  static constexpr GLenum RENDERBUFFER = GL_RENDERBUFFER_OES;
  static constexpr GLenum FRAMEBUFFER = GL_FRAMEBUFFER_OES;
  static constexpr GLenum COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0_OES;
  static constexpr GLenum DEPTH_ATTACHMENT = GL_DEPTH_ATTACHMENT_OES;
  static constexpr GLenum STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT_OES;
  static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL_OES;

  gcc_pure
  bool Initialise();

  void BindRenderbuffer(GLenum target, GLuint renderbuffer);
  void DeleteRenderbuffers(GLsizei n, GLuint *renderbuffers);
  void GenRenderbuffers(GLsizei n, GLuint *renderbuffers);
  void RenderbufferStorage(GLenum target, GLenum internalformat,
                           GLsizei width, GLsizei height);

  void BindFramebuffer(GLenum target, GLuint framebuffer);
  void DeleteFramebuffers(GLsizei n, GLuint *framebuffers);
  void GenFramebuffers(GLsizei n, GLuint *framebuffers);
  void FramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer);
  void FramebufferTexture2D(GLenum target, GLenum attachment,
                            GLenum textarget, GLuint texture,
                            GLint level);

#else
  /* on OpenGL, we assume that the extension is built-in */

  static constexpr GLenum RENDERBUFFER = GL_RENDERBUFFER_EXT;
  static constexpr GLenum FRAMEBUFFER = GL_FRAMEBUFFER_EXT;
  static constexpr GLenum COLOR_ATTACHMENT0 = GL_COLOR_ATTACHMENT0_EXT;
  static constexpr GLenum DEPTH_ATTACHMENT = GL_DEPTH_ATTACHMENT_EXT;
  static constexpr GLenum STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT_EXT;

#ifdef GL_DEPTH_STENCIL
  static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL;
#elif defined(GL_DEPTH_STENCIL_EXT)
  static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL_EXT;
#elif defined(GL_DEPTH_STENCIL_NV)
  static constexpr GLenum DEPTH_STENCIL = GL_DEPTH_STENCIL_NV;
#else
#error No GL_DEPTH_STENCIL found
#endif

  static inline bool
  Initialise()
  {
    return true;
  }

  static inline void
  BindRenderbuffer(GLenum target, GLuint renderbuffer)
  {
    glBindRenderbufferEXT(target, renderbuffer);
  }

  static inline void
  DeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers)
  {
    glDeleteRenderbuffersEXT(n, renderbuffers);
  }

  static inline void
  GenRenderbuffers(GLsizei n, GLuint *renderbuffers)
  {
    glGenRenderbuffersEXT(n, renderbuffers);
  }

  static inline void
  RenderbufferStorage(GLenum target, GLenum internalformat,
                      GLsizei width, GLsizei height)
  {
    glRenderbufferStorageEXT(target, internalformat, width, height);
  }

  static inline void
  BindFramebuffer(GLenum target, GLuint framebuffer)
  {
    glBindFramebufferEXT(target, framebuffer);
  }

  static inline void
  DeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
  {
    glDeleteFramebuffersEXT(n, framebuffers);
  }

  static inline void
  GenFramebuffers(GLsizei n, GLuint *framebuffers)
  {
    glGenFramebuffersEXT(n, framebuffers);
  }

  static inline void
  FramebufferRenderbuffer(GLenum target, GLenum attachment,
                          GLenum renderbuffertarget, GLuint renderbuffer)
  {
    glFramebufferRenderbufferEXT(target, attachment,
                                 renderbuffertarget, renderbuffer);
  }

  static inline void
  FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                       GLuint texture, GLint level)
  {
    glFramebufferTexture2DEXT(target, attachment, textarget, texture, level);
  }
#endif
}

#endif
