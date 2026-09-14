// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FBO.hpp"

/**
 * Wrapper for an OpenGL renderbuffer object.
 */
class GLRenderBuffer {
  GLuint id;

public:
  GLRenderBuffer() {
    Gen();
  }

  ~GLRenderBuffer() {
    Delete();
  }

  void Bind() {
    FBO::BindRenderbuffer(FBO::RENDERBUFFER, id);
  }

  static void Unbind() {
    FBO::BindRenderbuffer(FBO::RENDERBUFFER, GL_UNBIND_RENDERBUFFER);
  }

  static void Storage(GLenum internalformat,
                      GLsizei width, GLsizei height) {
    FBO::RenderbufferStorage(FBO::RENDERBUFFER, internalformat,
                             width, height);
  }

#ifdef HAVE_MULTISAMPLE_FBO
  /**
   * Like Storage(), but multisampled.  @a samples must not exceed
   * OpenGL::fbo_antialiasing_samples, and must match every other
   * attachment of the framebuffer this is attached to.
   */
  static void StorageMultisample(GLsizei samples, GLenum internalformat,
                                 GLsizei width, GLsizei height) {
    FBO::RenderbufferStorageMultisample(FBO::RENDERBUFFER, samples,
                                        internalformat, width, height);
  }
#endif

  void AttachFramebuffer(GLenum attachment) {
    FBO::FramebufferRenderbuffer(FBO::FRAMEBUFFER, attachment,
                                 FBO::RENDERBUFFER, id);
  }

  static void DetachFramebuffer(GLenum attachment) {
    FBO::FramebufferRenderbuffer(FBO::FRAMEBUFFER, attachment,
                                 FBO::RENDERBUFFER, GL_UNBIND_RENDERBUFFER);
  }

protected:
  void Gen() {
    FBO::GenRenderbuffers(1, &id);
  }

  void Delete() {
    FBO::DeleteRenderbuffers(1, &id);
  }
};
