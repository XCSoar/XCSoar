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

#ifdef USE_GLSL
#include "Shaders.hpp"
#include "Program.hpp"
#else
#include "Compatibility.hpp"
#endif

#ifdef HAVE_GLES1

/**
 * Emulate GL_CONSTANT_ALPHA on GLES1.
 */
static void
EmulateConstantAlpha(float alpha)
{
  /* configure a color with the given alpha value to be used as
     GL_PREVIOUS by glTexEnv(); its RGB values are ignored */
  glColor4f(0, 0, 0, alpha);

  /* enable "combine" mode */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

  /* RGB = texture.RGB */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

  /* A = glColor4f() */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

#endif

/**
 * Combine texture alpha and constant alpha.
 */
static void
CombineAlpha(float alpha)
{
#ifdef USE_GLSL
  glVertexAttrib4f(OpenGL::Attribute::COLOR,
                   1, 1, 1, alpha);

  OpenGL::combine_texture_shader->Use();
#else
  glColor4f(0, 0, 0, alpha);

  /* enable "combine" mode */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

  /* RGB = texture.RGB */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

  /* A = glColor4f() */
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
#endif
}

ScopeTextureConstantAlpha::ScopeTextureConstantAlpha(bool use_texture_alpha,
                                                     float alpha)
  :enabled(use_texture_alpha || alpha < 1.0f)
{
#ifdef USE_GLSL
  OpenGL::texture_shader->Use();
#endif

  if (!enabled) {
    /* opaque: use plain GL_REPLACE, avoid the alpha blending
       overhead */
#ifndef USE_GLSL
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
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

#ifdef HAVE_GLES1
    EmulateConstantAlpha(alpha);
#else
#ifndef USE_GLSL
    OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif

    /* tell OpenGL to use our alpha value instead of the texture's */
    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    glBlendColor(0, 0, 0, alpha);
#endif
  }
}

ScopeTextureConstantAlpha::~ScopeTextureConstantAlpha()
{
  if (enabled)
    glDisable(GL_BLEND);

#ifdef USE_GLSL
  /* restore default shader */
  OpenGL::solid_shader->Use();
#endif
}
