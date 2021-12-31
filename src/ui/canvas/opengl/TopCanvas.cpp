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

#include "ui/canvas/custom/TopCanvas.hpp"
#include "Globals.hpp"
#include "Init.hpp"
#include "Math/Point2D.hpp"

void
TopCanvas::SetupViewport(PixelSize native_size)
{
  auto new_size = OpenGL::SetupViewport(UnsignedPoint2D(native_size.width,
                                                        native_size.height));
  Canvas::Create(PixelSize(new_size.x, new_size.y));
}

void
TopCanvas::Resume()
{
#ifdef ANDROID
  surface = eglGetCurrentSurface(EGL_DRAW);
#endif
}

bool
TopCanvas::CheckResize(PixelSize new_native_size)
{
  if (new_native_size.width == OpenGL::window_size.x &&
      new_native_size.height == OpenGL::window_size.y)
    return false;

  SetupViewport(new_native_size);
  return true;
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopCanvas::SetDisplayOrientation(DisplayOrientation orientation)
{
  const auto native_size = GetNativeSize();
  if (native_size.width == 0 || native_size.height == 0)
    return;

  OpenGL::display_orientation = orientation;
  SetupViewport(PixelSize(OpenGL::window_size.x, OpenGL::window_size.y));
}

#endif
