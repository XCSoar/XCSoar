/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
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

#include "CanvasRotateShift.hpp"
#include "Shaders.hpp"
#include "Program.hpp"
#include "Math/Angle.hpp"
#include "ui/dim/Point.hpp"
#include "ui/opengl/System.hpp"
#include "Screen/Layout.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CanvasRotateShift::CanvasRotateShift(const PixelPoint pos, Angle angle,
                                     const int scale) noexcept
{
  glm::mat4 matrix = glm::rotate(glm::translate(glm::mat4(1),
                                                glm::vec3(pos.x, pos.y, 0)),
                                 GLfloat(angle.Radians()),
                                 glm::vec3(0, 0, 1));
  float gl_scale = scale / 100.f;
  if (Layout::ScaleSupported())
    gl_scale *= Layout::scale_1024 / 1024.f;
  matrix = glm::scale(matrix, glm::vec3(gl_scale));

  OpenGL::solid_shader->Use();
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(matrix));
}

CanvasRotateShift::~CanvasRotateShift() noexcept {
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4(1)));
}
