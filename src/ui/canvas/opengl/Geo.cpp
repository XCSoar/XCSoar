// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo.hpp"
#include "ui/opengl/System.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/FAISphere.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4
ToGLM(const WindowProjection &projection, const GeoPoint &reference) noexcept
{
  auto angle = projection.GetScreenAngle().Radians();
  auto scale = projection.GetScale();
  const PixelPoint &screen_origin = projection.GetScreenOrigin();
  const GeoPoint &screen_location = projection.GetGeoLocation();
  const GeoPoint projection_delta = reference - screen_location;

  const auto scale_r = scale * FAISphere::REARTH;
  const auto scale_x = scale_r * screen_location.latitude.fastcosine();
  const auto scale_y = -scale_r;

  const glm::vec3 scale_vec(GLfloat(scale_x), GLfloat(scale_y), 1);

  glm::mat4 matrix = glm::scale(glm::rotate(glm::translate(glm::mat4(1),
                                                           glm::vec3(screen_origin.x,
                                                                     screen_origin.y,
                                                                     0)),
                                            GLfloat(angle),
                                            glm::vec3(0, 0, -1)),
                                scale_vec);
  matrix = glm::translate(matrix,
                          glm::vec3(GLfloat(projection_delta.longitude.Native()),
                                    GLfloat(projection_delta.latitude.Native()),
                                    0.));
  return matrix;
}
