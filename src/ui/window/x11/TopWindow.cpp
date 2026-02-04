// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/display/Display.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/poll/Queue.hpp"
#include "util/Macros.hpp"

#include <X11/Xatom.h>

namespace UI {

void
TopWindow::CreateNative(const TCHAR *text, PixelSize size,
                        TopWindowStyle style)
{
  const auto x_display = display.GetXDisplay();

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
  int depth = CopyFromParent;
  Visual *visual = CopyFromParent;

  x_window = XCreateWindow(x_display, x_root,
                           0, 0, size.width, size.height, 0,
                           depth, InputOutput,
                           visual, valuemask,
                           &swa);
  if (x_window == 0)
    throw std::runtime_error("XCreateWindow() failed");

  if (XClassHint *class_hint = XAllocClassHint()) {
    class_hint->res_name = class_hint->res_class = const_cast<char *>("xcsoar");
    XSetClassHint(x_display, x_window, class_hint);
    XFree(class_hint);
  }

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
