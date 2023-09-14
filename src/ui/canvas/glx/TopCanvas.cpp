// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/display/Display.hpp"
#include "ui/dim/Size.hpp"

#include <stdexcept>

TopCanvas::TopCanvas(UI::Display &_display,
                     X11Window x_window)
  :display(_display)
{
  glx_window = glXCreateWindow(display.GetXDisplay(), _display.GetFBConfig(),
                               x_window, nullptr);
  XSync(display.GetXDisplay(), false);

  if (!glXMakeContextCurrent(display.GetXDisplay(), glx_window, glx_window,
                             display.GetGLXContext()))
    throw std::runtime_error("Failed to attach GLX context to GLX window");

  const PixelSize effective_size = GetNativeSize();
  if (effective_size.width == 0 || effective_size.height == 0)
    throw std::runtime_error("Failed to query GLX drawable size");

  SetupViewport(effective_size);
}

TopCanvas::~TopCanvas() noexcept
{
  glXDestroyWindow(display.GetXDisplay(), glx_window);
}

PixelSize
TopCanvas::GetNativeSize() const noexcept
{
  unsigned w = 0, h = 0;
  glXQueryDrawable(display.GetXDisplay(), glx_window, GLX_WIDTH, &w);
  glXQueryDrawable(display.GetXDisplay(), glx_window, GLX_HEIGHT, &h);
  if (w <= 0 || h <= 0)
    return PixelSize(0, 0);

  return PixelSize(w, h);
}

void
TopCanvas::Flip()
{
  glXSwapBuffers(display.GetXDisplay(), glx_window);
}
