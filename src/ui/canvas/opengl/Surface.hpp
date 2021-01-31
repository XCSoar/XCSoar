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

#ifndef XCSOAR_SCREEN_OPENGL_SURFACE_HPP
#define XCSOAR_SCREEN_OPENGL_SURFACE_HPP

struct GLSurfaceListener {
  virtual void SurfaceCreated() = 0;
  virtual void SurfaceDestroyed() = 0;
};

/**
 * This flag is true when there is a valid OpenGL surface.
 */
extern bool surface_valid;

void
AddSurfaceListener(GLSurfaceListener &listener);

void
RemoveSurfaceListener(GLSurfaceListener &listener);

/**
 * Notify all listeners that the OpenGL surface has been created.
 */
void
SurfaceCreated();

/**
 * Notify all listeners that the OpenGL surface has been destroyed.
 */
void
SurfaceDestroyed();

#endif
