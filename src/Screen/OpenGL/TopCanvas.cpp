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

#include "Screen/Custom/TopCanvas.hpp"
#include "Globals.hpp"
#include "Init.hpp"
#include "Math/Point2D.hpp"

void
TopCanvas::SetupViewport(PixelSize native_size)
{
  auto new_size = OpenGL::SetupViewport(UnsignedPoint2D(native_size.cx,
                                                        native_size.cy));
  Canvas::Create(PixelSize(new_size.x, new_size.y));
}

void
TopCanvas::Resume()
{
#if defined(ANDROID) && defined(USE_EGL)
  surface = eglGetCurrentSurface(EGL_DRAW);
#endif

  OpenGL::SetupContext();
  OpenGL::SetupViewport(UnsignedPoint2D(size.cx, size.cy));
}

bool
TopCanvas::CheckResize(PixelSize new_native_size)
{
  if ((unsigned)new_native_size.cx == OpenGL::window_size.x &&
      (unsigned)new_native_size.cy == OpenGL::window_size.y)
    return false;

  SetupViewport(new_native_size);
  return true;
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopCanvas::SetDisplayOrientation(DisplayOrientation orientation)
{
  const auto native_size = GetNativeSize();
  if (native_size.cx <= 0 || native_size.cy <= 0)
    return;

  OpenGL::display_orientation = orientation;
  SetupViewport(PixelSize(OpenGL::window_size.x, OpenGL::window_size.y));
}

#endif
