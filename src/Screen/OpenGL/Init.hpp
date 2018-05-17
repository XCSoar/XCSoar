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

#ifndef XCSOAR_SCREEN_OPENGL_INIT_HPP
#define XCSOAR_SCREEN_OPENGL_INIT_HPP

#include <stdint.h>

struct UnsignedPoint2D;
enum class DisplayOrientation : uint8_t;

namespace OpenGL {
  /**
   * Initialize our OpenGL library.  Call when XCSoar starts.
   */
  void Initialise();

  /**
   * Set up our OpenGL library.  Call after the video mode and the
   * OpenGL context have been set up.
   */
  void SetupContext();

  /**
   * Set up the viewport and the matrices for 2D drawing.  Apply the
   * #DisplayOrientation via glRotatef() (OpenGL projection matrix).
   *
   * @param size the native screen size in pixels
   * @return the logical screen size (after rotation)
   */
  UnsignedPoint2D SetupViewport(UnsignedPoint2D size);

  /**
   * Deinitialize our OpenGL library.  Call before shutdown.
   */
  void Deinitialise();
};

#endif
