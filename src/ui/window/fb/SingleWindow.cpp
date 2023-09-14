// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/window/SingleWindow.hpp"
#include "ui/event/shared/Event.hpp"

namespace UI {

bool
SingleWindow::FilterEvent(const UI::Event &event, Window *allowed) const noexcept
{
  assert(allowed != nullptr);

  switch (event.type) {
  case UI::Event::MOUSE_MOTION:
  case UI::Event::MOUSE_DOWN:
  case UI::Event::MOUSE_UP:
    return FilterMouseEvent(event.point, allowed);

  default:
    return true;
  }
}

} // namespace UI
