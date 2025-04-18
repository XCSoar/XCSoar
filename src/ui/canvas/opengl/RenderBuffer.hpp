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
