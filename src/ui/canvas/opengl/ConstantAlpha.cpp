// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConstantAlpha.hpp"
#include "Shaders.hpp"
#include "Program.hpp"
#include "Attribute.hpp"
#include "ui/opengl/System.hpp"

/**
 * Combine texture alpha and constant alpha.
 */
static void
CombineAlpha(float alpha)
{
  glVertexAttrib4f(OpenGL::Attribute::COLOR,
                   1, 1, 1, alpha);

  OpenGL::combine_texture_shader->Use();
}

ScopeTextureConstantAlpha::ScopeTextureConstantAlpha(bool use_texture_alpha,
                                                     float alpha)
  :enabled(use_texture_alpha || alpha < 1.0f)
{
  OpenGL::texture_shader->Use();

  if (!enabled) {
    /* opaque: use plain GL_REPLACE, avoid the alpha blending
       overhead */
    return;
  }

  glEnable(GL_BLEND);

  if (use_texture_alpha) {
    if (alpha >= 1.0f) {
      /* use only texture alpha */

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
      /* combine texture alpha and constant alpha */

      CombineAlpha(alpha);
    }
  } else {
    /* use only constant alpha, ignore texture alpha */

    /* tell OpenGL to use our alpha value instead of the texture's */
    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    glBlendColor(0, 0, 0, alpha);
  }
}

ScopeTextureConstantAlpha::~ScopeTextureConstantAlpha()
{
  if (enabled)
    glDisable(GL_BLEND);

  /* restore default shader */
  OpenGL::solid_shader->Use();
}
