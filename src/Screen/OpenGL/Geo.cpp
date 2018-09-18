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

#include "Geo.hpp"
#include "System.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/FAISphere.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4
ToGLM(const WindowProjection &projection, const GeoPoint &reference)
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
