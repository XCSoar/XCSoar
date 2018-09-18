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

#ifndef XCSOAR_SCREEN_OPENGL_CANVAS_ROTATE_SHIFT_HPP
#define XCSOAR_SCREEN_OPENGL_CANVAS_ROTATE_SHIFT_HPP

#include "Math/Angle.hpp"
#include "Screen/Point.hpp"
#include "Screen/Layout.hpp"
#include "System.hpp"
#include "Shaders.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * Temporarily changes the transformation matrix. Meant as replacement
 * for PolygonRotateShift().
 *
 * WARNING: Pen widths also get scaled!
 */
class CanvasRotateShift
{
public:
  CanvasRotateShift(const PixelPoint pos, Angle angle,
                    const int scale = 100) {
    glm::mat4 matrix = glm::rotate(glm::translate(glm::mat4(1),
                                                  glm::vec3(pos.x, pos.y, 0)),
                                   GLfloat(angle.Radians()),
                                   glm::vec3(0, 0, 1));
    float gl_scale = scale / 100.f;
    if (Layout::ScaleSupported())
      gl_scale *= Layout::scale_1024 / 1024.f;
    matrix = glm::scale(matrix, glm::vec3(gl_scale));
    glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                       glm::value_ptr(matrix));
  };

  ~CanvasRotateShift() {
    glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(1)));
  }
};

#endif
