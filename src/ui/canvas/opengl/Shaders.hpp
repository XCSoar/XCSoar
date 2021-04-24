/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
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

#ifndef XCSOAR_SCREEN_OPENGL_SHADERS_HPP
#define XCSOAR_SCREEN_OPENGL_SHADERS_HPP

#include "ui/opengl/System.hpp"

class GLProgram;

namespace OpenGL {

/**
 * A shader that draws a solid color (#Attribute::COLOR).
 */
extern GLProgram *solid_shader;
extern GLint solid_projection, solid_modelview, solid_translate;

/**
 * A shader that copies the texture.
 */
extern GLProgram *texture_shader;
extern GLint texture_projection, texture_texture, solid_translate;

/**
 * A shader that copies the inverted texture.
 */
extern GLProgram *invert_shader;
extern GLint invert_projection, invert_texture, invert_translate;

/**
 * A shader that copies the texture's alpha channel, but replaces
 * the color (#Attribute::COLOR).
 */
extern GLProgram *alpha_shader;
extern GLint alpha_projection, alpha_texture, alpha_translate;

/**
 * A shader that multiplies the texture with #Attribute::COLOR.
 */
extern GLProgram *combine_texture_shader;
extern GLint combine_texture_projection, combine_texture_texture,
  combine_texture_translate;

/**
 * Throws on error.
 */
void InitShaders();

void DeinitShaders() noexcept;

void UpdateShaderProjectionMatrix() noexcept;

void
UpdateShaderTranslate() noexcept;

} // namespace OpenGL

#endif
