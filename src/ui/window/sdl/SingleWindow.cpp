// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../SingleWindow.hpp"
#include "../Features.hpp"
#include "ui/event/sdl/Event.hpp"

#include <SDL_events.h>

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
#ifdef HAVE_HIGHDPI_SUPPORT
    {
      return FilterMouseEvent(PointToReal(PixelPoint(event.button.x, event.button.y)), allowed);
    }
#else
    return FilterMouseEvent(PixelPoint(event.button.x, event.button.y),
                            allowed);
#endif

  default:
    return true;
  }
}

} // namespace UI
