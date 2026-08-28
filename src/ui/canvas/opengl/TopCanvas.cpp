// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/TopCanvas.hpp"
#include "Canvas.hpp"
#include "Globals.hpp"
#include "Init.hpp"
#include "Math/Point2D.hpp"

#ifdef USE_EGL
#include "ui/display/Display.hpp"

#include <algorithm>
#endif

PixelSize
TopCanvas::GetSize() const noexcept
{
  return {OpenGL::viewport_size.x, OpenGL::viewport_size.y};
}

unsigned
TopCanvas::GetPresentationBufferCount() const noexcept
{
  unsigned count = presentation_buffer_count;

#ifdef USE_EGL
  if (surface != EGL_NO_SURFACE) {
#ifdef EGL_BUFFER_AGE_KHR
    const EGLint age_attr = EGL_BUFFER_AGE_KHR;
#else
    const EGLint age_attr = 0x313D;
#endif
    auto age = display.QuerySurface(surface, age_attr);
    if (age && *age > 0)
      count = std::max(count, unsigned(*age));
  }
#endif

  if (count < 3)
    count = 3;
  else if (count > 8)
    count = 8;

  return count;
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
#ifdef ANDROID
  if (IsReady()) {
    if (new_native_size.width == OpenGL::window_size.x &&
        new_native_size.height == OpenGL::window_size.y)
      return false;

    /* When the Activity handles configChanges (orientation|screenSize),
       Android resizes the SurfaceView without destroying it.  The EGL
       surface keeps the old buffer dimensions, so we must recreate it
       to pick up the new native window size. */
    const PixelSize egl_size = GetNativeSize();
    if (egl_size.width != new_native_size.width ||
        egl_size.height != new_native_size.height) {
      ReleaseSurface();
      if (!AcquireSurface())
        /* AcquireSurface() already called SetupViewport() with the
           correct dimensions from the new native window */
        return false;
    } else {
      SetupViewport(new_native_size);
    }

    return true;
  }
#endif
  
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
