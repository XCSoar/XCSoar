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

  PixelPoint p{};

  switch (event.type) {
  case SDL_MOUSEMOTION:
#ifdef HAVE_HIGHDPI_SUPPORT
      p = PointToReal(PixelPoint(event.motion.x, event.motion.y));
#else
      p = PixelPoint(event.motion.x, event.motion.y);
#endif
    break;

  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
#ifdef HAVE_HIGHDPI_SUPPORT
    p = PointToReal(PixelPoint(event.button.x, event.button.y));
#else
    p = PixelPoint(event.button.x, event.button.y);
#endif
    break;

  default:
    return true;
  }

  #if defined(ENABLE_OPENGL) && defined(SOFTWARE_ROTATE_DISPLAY)
  if(OpenGL::window_size.x > 0 && OpenGL::window_size.y > 0)
    p = TransformCoordinates(p, PixelSize{OpenGL::window_size.x,
                                          OpenGL::window_size.y});
#endif

  return FilterMouseEvent(p, allowed);
}

} // namespace UI
