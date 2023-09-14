// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CanvasRotateShift.hpp"
#include "Shaders.hpp"
#include "Program.hpp"
#include "Math/Angle.hpp"
#include "ui/dim/Point.hpp"
#include "ui/opengl/System.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CanvasRotateShift::CanvasRotateShift(const PixelPoint pos, Angle angle,
                                     const float scale) noexcept
{
  glm::mat4 matrix = glm::rotate(glm::translate(glm::mat4(1),
                                                glm::vec3(pos.x, pos.y, 0)),
                                 GLfloat(angle.Radians()),
                                 glm::vec3(0, 0, 1));
  matrix = glm::scale(matrix, glm::vec3{scale});

  OpenGL::solid_shader->Use();
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(matrix));
}

CanvasRotateShift::~CanvasRotateShift() noexcept {
  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4(1)));
}
