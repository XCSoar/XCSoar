// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/System.hpp"

/**
 * Enables and auto-disables an OpenGL capability.
 */
template<GLenum cap>
class GLEnable {
public:
  [[nodiscard]]
  GLEnable() noexcept {
    ::glEnable(cap);
  }

  ~GLEnable() noexcept {
    ::glDisable(cap);
  }

  GLEnable(const GLEnable &) = delete;
  GLEnable &operator=(const GLEnable &) = delete;
};

class GLBlend : public GLEnable<GL_BLEND> {
public:
  [[nodiscard]]
  GLBlend(GLenum sfactor, GLenum dfactor) noexcept {
    ::glBlendFunc(sfactor, dfactor);
  }

  [[nodiscard]]
  GLBlend(GLclampf alpha) noexcept {
    ::glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    ::glBlendColor(0, 0, 0, alpha);
  }
};

/**
 * Enable alpha blending with source's alpha value (the most common
 * variant of GL_BLEND).
 */
class ScopeAlphaBlend : GLBlend {
public:
  [[nodiscard]]
  ScopeAlphaBlend() noexcept:GLBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) {}
};

class GLScissor : public GLEnable<GL_SCISSOR_TEST> {
public:
  [[nodiscard]]
  GLScissor(GLint x, GLint y, GLsizei width, GLsizei height) noexcept {
    ::glScissor(x, y, width, height);
  }
};
