// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../SingleWindow.hpp"
#include "../Features.hpp"
#include "ui/event/sdl/Event.hpp"

#include <SDL_events.h>

#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
#include "ui/event/shared/TransformCoordinates.hpp"
#endif

#include <cassert>

namespace UI {

bool
SingleWindow::FilterEvent(const UI::Event &_event, Window *allowed) const noexcept
{
  assert(allowed != nullptr);

  const SDL_Event &event = _event.event;

  switch (event.type) {
  case SDL_MOUSEMOTION:
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    {
#ifdef HAVE_HIGHDPI_SUPPORT
      auto p = PointToReal(PixelPoint(event.button.x, event.button.y));
#else
      auto p = PixelPoint(event.button.x, event.button.y);
#endif
#if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
      p = TransformCoordinates(p, PixelSize{OpenGL::window_size.x,
                                            OpenGL::window_size.y});
#endif
      return FilterMouseEvent(p, allowed);
    }

  default:
    return true;
  }
}

} // namespace UI
