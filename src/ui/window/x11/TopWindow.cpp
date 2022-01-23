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

#include "../TopWindow.hpp"
#include "ui/display/Display.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "util/Macros.hpp"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#ifdef USE_GLX
#include "ui/glx/System.hpp"
#endif

#include <X11/Xatom.h>

namespace UI {

void
TopWindow::CreateNative(const TCHAR *text, PixelSize size,
                        TopWindowStyle style)
{
  const auto x_display = display.GetXDisplay();
#ifdef USE_GLX
  XVisualInfo *vi = glXGetVisualFromFBConfig(display.GetXDisplay(),
                                             display.GetFBConfig());
#endif

  const auto x_root = DefaultRootWindow(x_display);
  if (x_root == 0)
    throw std::runtime_error("DefaultRootWindow() failed");

  XSetWindowAttributes swa;
  swa.event_mask = KeyPressMask | KeyReleaseMask | KeymapStateMask |
    ButtonPressMask | ButtonReleaseMask |
    PointerMotionMask |
    VisibilityChangeMask |
    ExposureMask | StructureNotifyMask;

  unsigned long valuemask = CWEventMask;
#ifdef USE_GLX
  int depth = vi->depth;
  Visual *visual = vi->visual;
  swa.border_pixel = 0;
  swa.background_pixel = 0;
  swa.colormap = XCreateColormap(x_display, x_root,
                                 vi->visual, AllocNone);
  valuemask |= CWBorderPixel|CWBackPixel|CWColormap;
#else
  int depth = CopyFromParent;
  Visual *visual = CopyFromParent;
#endif

  x_window = XCreateWindow(x_display, x_root,
                           0, 0, size.width, size.height, 0,
                           depth, InputOutput,
                           visual, valuemask,
                           &swa);
  if (x_window == 0)
    throw std::runtime_error("XCreateWindow() failed");

#ifdef USE_GLX
  XFree(vi);
#endif

  XMapWindow(x_display, x_window);
  XStoreName(x_display, x_window, text);

  if (style.GetFullScreen()) {
    /* tell the window manager we want full-screen */
    const Atom atoms[] = {
      XInternAtom(x_display, "_NET_WM_STATE_FULLSCREEN", false),
    };
    XChangeProperty(x_display, x_window,
                    XInternAtom(x_display, "_NET_WM_STATE", false),
                    XA_ATOM, 32, PropModeReplace,
                    (const unsigned char *)atoms, ARRAY_SIZE(atoms));
  }

  /* receive "Close" button clicks from the window manager */
  auto wm_delete_window = XInternAtom(x_display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(x_display, x_window, &wm_delete_window, 1);
}

bool
TopWindow::IsVisible() const noexcept
{
  return UI::event_queue->IsVisible();
}

void
TopWindow::EnableCapture() noexcept
{
  XGrabPointer(display.GetXDisplay(), x_window, true,
               ButtonPressMask |
               ButtonReleaseMask |
               PointerMotionMask,
               GrabModeAsync, GrabModeAsync,
               0, 0, CurrentTime);
}

void
TopWindow::DisableCapture() noexcept
{
  XUngrabPointer(display.GetXDisplay(), CurrentTime);
}

} // namespace UI
