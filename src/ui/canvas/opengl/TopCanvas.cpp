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
#include "Canvas.hpp"
#include "Globals.hpp"
#include "Init.hpp"
#include "Math/Point2D.hpp"

PixelSize
TopCanvas::GetSize() const noexcept
{
  return {OpenGL::viewport_size.x, OpenGL::viewport_size.y};
}

PixelSize
TopCanvas::SetupViewport(PixelSize native_size) noexcept
{
  auto new_size = OpenGL::SetupViewport(UnsignedPoint2D(native_size.width,
                                                        native_size.height));
  return PixelSize{new_size.x, new_size.y};
}

bool
TopCanvas::CheckResize(PixelSize new_native_size) noexcept
{
  if (new_native_size.width == OpenGL::window_size.x &&
      new_native_size.height == OpenGL::window_size.y)
    return false;

  SetupViewport(new_native_size);
  return true;
}

#ifdef SOFTWARE_ROTATE_DISPLAY

PixelSize
TopCanvas::SetDisplayOrientation(DisplayOrientation orientation) noexcept
{
  OpenGL::display_orientation = orientation;
  return SetupViewport({OpenGL::window_size.x, OpenGL::window_size.y});
}

#endif

Canvas
TopCanvas::Lock()
{
  return Canvas{GetSize()};
}

void
TopCanvas::Unlock() noexcept
{
}
