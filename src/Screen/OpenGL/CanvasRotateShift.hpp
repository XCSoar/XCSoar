/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

/**
 * Temporarily changes the transformation matrix. Meant as replacement
 * for PolygonRotateShift().
 *
 * WARNING: Pen widths also get scaled!
 */
class CanvasRotateShift
{
public:
  CanvasRotateShift(const RasterPoint pos, Angle angle,
                    const int scale = 100) {
    glPushMatrix();

#ifdef HAVE_GLES
    glTranslatex((GLfixed)pos.x << 16, (GLfixed)pos.y << 16, 0);
#ifdef FIXED_MATH
    GLfixed fixed_angle = angle.Degrees().as_glfixed();
#else
    GLfixed fixed_angle = angle.Degrees() * (1<<16);
#endif
    glRotatex(fixed_angle, 0, 0, 1<<16);
#else
    glTranslatef(pos.x, pos.y, 0.);
    glRotatef((GLfloat)angle.Degrees(), 0., 0., 1.);
#endif

#ifdef HAVE_GLES
    GLfixed gl_scale = ((GLfixed) scale << 16) / 100;
    if (Layout::ScaleSupported())
      gl_scale = (gl_scale * Layout::scale_1024) >> 10;
    glScalex(gl_scale, gl_scale, (GLfixed)1 << 16);
#else
    float gl_scale = scale / 100.f;
    if (Layout::ScaleSupported())
      gl_scale *= Layout::scale_1024 / 1024.f;
    glScalef(gl_scale, gl_scale, 1.);
#endif
  };

  ~CanvasRotateShift() {
    glPopMatrix();
  }
};

#endif
