/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "FBO.hpp"

#ifdef HAVE_GLES

#include <assert.h>
#include <dlfcn.h>

static PFNGLBINDRENDERBUFFEROESPROC _glBindRenderbuffer;
static PFNGLDELETERENDERBUFFERSOESPROC _glDeleteRenderbuffers;
static PFNGLGENRENDERBUFFERSOESPROC _glGenRenderbuffers;
static PFNGLRENDERBUFFERSTORAGEOESPROC _glRenderbufferStorage;

static PFNGLBINDFRAMEBUFFEROESPROC _glBindFramebuffer;
static PFNGLDELETEFRAMEBUFFERSOESPROC _glDeleteFramebuffers;
static PFNGLGENFRAMEBUFFERSOESPROC _glGenFramebuffers;
static PFNGLFRAMEBUFFERRENDERBUFFEROESPROC _glFramebufferRenderbuffer;
static PFNGLFRAMEBUFFERTEXTURE2DOESPROC _glFramebufferTexture2D;

bool
FBO::Initialise()
{
  _glBindRenderbuffer = (PFNGLBINDRENDERBUFFEROESPROC)
    dlsym(RTLD_DEFAULT, "glBindRenderbufferOES");
  _glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSOESPROC)
    dlsym(RTLD_DEFAULT, "glDeleteRenderbuffersOES");
  _glGenRenderbuffers = (PFNGLGENRENDERBUFFERSOESPROC)
    dlsym(RTLD_DEFAULT, "glGenRenderbuffersOES");
  _glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEOESPROC)
    dlsym(RTLD_DEFAULT, "glRenderbufferStorageOES");

  _glBindFramebuffer = (PFNGLBINDFRAMEBUFFEROESPROC)
    dlsym(RTLD_DEFAULT, "glBindFramebufferOES");
  _glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSOESPROC)
    dlsym(RTLD_DEFAULT, "glDeleteFramebuffersOES");
  _glGenFramebuffers = (PFNGLGENFRAMEBUFFERSOESPROC)
    dlsym(RTLD_DEFAULT, "glGenFramebuffersOES");
  _glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFEROESPROC)
    dlsym(RTLD_DEFAULT, "glFramebufferRenderbufferOES");
  _glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DOESPROC)
    dlsym(RTLD_DEFAULT, "glFramebufferTexture2DOES");

  return _glBindRenderbuffer != nullptr && _glDeleteRenderbuffers != nullptr &&
    _glGenRenderbuffers != nullptr && _glRenderbufferStorage != nullptr &&
    _glBindFramebuffer != nullptr && _glDeleteFramebuffers != nullptr &&
    _glGenFramebuffers != nullptr && _glFramebufferRenderbuffer != nullptr &&
    _glFramebufferTexture2D != nullptr;
}

void
FBO::BindRenderbuffer(GLenum target, GLuint renderbuffer)
{
  assert(_glBindRenderbuffer != nullptr);

  _glBindRenderbuffer(target, renderbuffer);
}

void
FBO::DeleteRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
  assert(_glDeleteRenderbuffers != nullptr);

  _glDeleteRenderbuffers(n, renderbuffers);
}

void
FBO::GenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
  assert(_glGenRenderbuffers != nullptr);

  _glGenRenderbuffers(n, renderbuffers);
}

void
FBO::RenderbufferStorage(GLenum target, GLenum internalformat,
                         GLsizei width, GLsizei height)
{
  assert(_glRenderbufferStorage != nullptr);

  _glRenderbufferStorage(target, internalformat, width, height);
}

void
FBO::BindFramebuffer(GLenum target, GLuint framebuffer)
{
  assert(_glBindFramebuffer != nullptr);

  _glBindFramebuffer(target, framebuffer);
}

void
FBO::DeleteFramebuffers(GLsizei n, GLuint *framebuffers)
{
  assert(_glDeleteFramebuffers != nullptr);

  _glDeleteFramebuffers(n, framebuffers);
}

void
FBO::GenFramebuffers(GLsizei n, GLuint *framebuffers)
{
  assert(_glGenFramebuffers != nullptr);

  _glGenFramebuffers(n, framebuffers);
}

void
FBO::FramebufferRenderbuffer(GLenum target, GLenum attachment,
                             GLenum renderbuffertarget, GLuint renderbuffer)
{
  assert(_glFramebufferRenderbuffer != nullptr);

  _glFramebufferRenderbuffer(target, attachment,
                                 renderbuffertarget, renderbuffer);
}

void
FBO::FramebufferTexture2D(GLenum target, GLenum attachment,
                          GLenum textarget, GLuint texture,
                          GLint level)
{
  assert(_glFramebufferTexture2D != nullptr);

  _glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

#endif
