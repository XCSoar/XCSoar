// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FBO.hpp"

/**
 * Wrapper for an OpenGL framebuffer object.
 */
class GLFrameBuffer {
  GLuint id;

public:
  GLFrameBuffer() noexcept {
    Gen();
  }

  ~GLFrameBuffer() noexcept {
    Delete();
  }

  void Bind() noexcept {
    FBO::BindFramebuffer(FBO::FRAMEBUFFER, id);
  }

  static void Unbind() noexcept {

    FBO::BindFramebuffer(FBO::FRAMEBUFFER, GL_UNBIND_FRAMEBUFFER);
  }

protected:
  void Gen() noexcept {
    FBO::GenFramebuffers(1, &id);
  }

  void Delete() noexcept {
    FBO::DeleteFramebuffers(1, &id);
  }
};
