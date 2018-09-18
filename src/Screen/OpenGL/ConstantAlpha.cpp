/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "ConstantAlpha.hpp"
#include "System.hpp"
#include "Shaders.hpp"
#include "Program.hpp"

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
