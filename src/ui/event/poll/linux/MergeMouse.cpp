// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MergeMouse.hpp"
#include "ui/event/shared/Event.hpp"

namespace UI {

void
MergeMouse::SetScreenSize(PixelSize screen_size) noexcept
{
  if (screen_size != rotate.GetScreenSize()) {
    x = screen_size.width / 2;
    y = screen_size.height / 2;
  }

  rotate.SetScreenSize(screen_size);
}

void
MergeMouse::SetDown(bool new_down) noexcept
{
  if (new_down != down) {
    down = new_down;
    if (down)
      pressed = true;
    else
      released = true;
  }
}

void
MergeMouse::MoveAbsolute(PixelPoint p) noexcept
{
  p = rotate.DoAbsolute(p);

  const auto screen_size = rotate.GetScreenSize();

  if (screen_size.width > 0) {
    if (p.x < 0)
      p.x = 0;
    else if (unsigned(p.x) > screen_size.width)
      p.x = screen_size.width - 1;

    if (unsigned(p.x) != x) {
      x = p.x;
      moved = true;
    }
  }

  if (screen_size.height > 0) {
    if (p.y < 0)
      p.y = 0;
    else if (unsigned(p.y) > screen_size.height)
      p.y = screen_size.height - 1;

    if (unsigned(p.y) != y) {
      y = p.y;
      moved = true;
    }
  }
}

void
MergeMouse::MoveAbsolute(int new_x, int new_y,
                         int min_x, int max_x, int min_y, int max_y) noexcept
{
  /* scale touschreen coordinates to screen size */

  if (new_x < min_x)
    new_x = 0;
  else if (max_x > min_x)
    new_x = new_x * int(rotate.GetScreenSize().width) / (max_x - min_x);

  if (new_y < min_y)
    new_y = 0;
  else if (max_y > min_y)
    new_y = new_y * int(rotate.GetScreenSize().height) / (max_y - min_y);

  /* now call the "real" MoveAbsolute() */
  MoveAbsolute(PixelPoint(new_x, new_y));
}

void
MergeMouse::MoveRelative(PixelPoint d) noexcept
{
  MoveAbsolute(GetPosition() + rotate.DoRelative(d));
}

Event
MergeMouse::Generate() noexcept
{
  if (moved) {
    moved = false;
    return Event(Event::MOUSE_MOTION, PixelPoint(x, y));
  }

  if (pressed) {
    pressed = false;
    return Event(Event::MOUSE_DOWN, PixelPoint(x, y));
  }

  if (released) {
    released = false;
    return Event(Event::MOUSE_UP, PixelPoint(x, y));
  }

  if (wheel != 0) {
    Event event(Event::MOUSE_WHEEL, PixelPoint(x, y));
    event.param = unsigned(wheel);
    wheel = 0;
    return event;
  }

  return Event(Event::Type::NOP);
}

} // namespace UI
