// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/event/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "ui/dim/Size.hpp"

#ifdef USE_X11
#include "ui/canvas/custom/TopCanvas.hpp"
#endif

namespace UI {

void
TopWindow::OnResize(PixelSize new_size) noexcept
{
  event_queue->SetScreenSize(new_size);

  ContainerWindow::OnResize(new_size);
}

bool
TopWindow::OnEvent(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::CALLBACK:
    break;

  case Event::CLOSE:
    OnClose();
    break;

  case Event::KEY_DOWN:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    return w->OnKeyDown(event.param);

  case Event::KEY_UP:
    w = GetFocusedWindow();
    if (w == nullptr)
      w = this;

    return w->OnKeyUp(event.param);

  case Event::MOUSE_MOTION:
#ifdef DRAW_MOUSE_CURSOR
    cursor_visible_until = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    /* redraw to update the mouse cursor position */
    Invalidate();
#endif

    // XXX keys
    return OnMouseMove(event.point, 0);

  case Event::MOUSE_DOWN:
    return double_click.Check(event.point)
      ? OnMouseDouble(event.point)
      : OnMouseDown(event.point);

  case Event::MOUSE_UP:
    double_click.Moved(event.point);

    return OnMouseUp(event.point);

  case Event::MOUSE_WHEEL:
    return OnMouseWheel(event.point, (int)event.param);

#ifdef USE_X11
  case Event::RESIZE:
    if (screen->CheckResize(PixelSize(event.point.x, event.point.y)))
      Resize(screen->GetSize());
    return true;
#endif

#if defined(USE_X11) || defined(MESA_KMS)
  case Event::EXPOSE:
    Invalidate();
    return true;
#endif
  }

  return false;
}

} // namespace UI
