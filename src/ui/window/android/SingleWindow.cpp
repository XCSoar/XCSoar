// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../SingleWindow.hpp"
#include "ui/event/shared/Event.hpp"

namespace UI {

bool
SingleWindow::FilterEvent(const Event &event, Window *allowed) const noexcept
{
  assert(allowed != nullptr);

  switch (event.type) {
  case Event::MOUSE_MOTION:
  case Event::MOUSE_DOWN:
  case Event::MOUSE_UP:
    return FilterMouseEvent(event.point, allowed);

  default:
    return true;
  }
}

} // namespace UI
