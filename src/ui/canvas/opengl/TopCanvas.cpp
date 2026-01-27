// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  return SetupViewport(native_size, PixelSize(0, 0));
}

PixelSize
TopCanvas::SetupViewport(PixelSize native_size,
                         PixelSize content_size) noexcept
{
  auto new_size = OpenGL::SetupViewport(
    UnsignedPoint2D(native_size.width, native_size.height),
    UnsignedPoint2D(content_size.width, content_size.height));
  return PixelSize{new_size.x, new_size.y};
}

bool
TopCanvas::CheckResize(PixelSize new_native_size) noexcept
{
  return CheckResize(new_native_size, PixelSize(0, 0));
}

bool
TopCanvas::CheckResize(PixelSize new_native_size,
                       PixelSize content_size) noexcept
{
  if (new_native_size.width == OpenGL::window_size.x &&
      new_native_size.height == OpenGL::window_size.y)
    return false;

  const auto effective_content =
    (content_size.width > 0 && content_size.height > 0)
      ? content_size
      : new_native_size;
  SetupViewport(new_native_size, effective_content);
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
