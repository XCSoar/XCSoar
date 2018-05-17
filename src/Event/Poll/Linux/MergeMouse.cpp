/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "MergeMouse.hpp"
#include "Event/Shared/Event.hpp"

void
MergeMouse::SetScreenSize(unsigned width, unsigned height)
{
  if (width != rotate.GetWidth())
    x = width / 2;

  if (height != rotate.GetHeight())
    y = height / 2;

  rotate.SetSize(width, height);
}

void
MergeMouse::SetDown(bool new_down)
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
MergeMouse::MoveAbsolute(PixelPoint p)
{
  p = rotate.DoAbsolute(p);

  const unsigned screen_width = rotate.GetWidth();
  if (screen_width > 0) {
    if (p.x < 0)
      p.x = 0;
    else if (unsigned(p.x) > screen_width)
      p.x = screen_width - 1;

    if (unsigned(p.x) != x) {
      x = p.x;
      moved = true;
    }
  }

  const unsigned screen_height = rotate.GetHeight();
  if (screen_height > 0) {
    if (p.y < 0)
      p.y = 0;
    else if (unsigned(p.y) > screen_height)
      p.y = screen_height - 1;

    if (unsigned(p.y) != y) {
      y = p.y;
      moved = true;
    }
  }
}

void
MergeMouse::MoveAbsolute(int new_x, int new_y,
                         int min_x, int max_x, int min_y, int max_y)
{
  /* scale touschreen coordinates to screen size */

  if (new_x < min_x)
    new_x = 0;
  else if (max_x > min_x)
    new_x = new_x * int(rotate.GetWidth()) / (max_x - min_x);

  if (new_y < min_y)
    new_y = 0;
  else if (max_y > min_y)
    new_y = new_y * int(rotate.GetHeight()) / (max_y - min_y);

  /* now call the "real" MoveAbsolute() */
  MoveAbsolute(PixelPoint(new_x, new_y));
}

void
MergeMouse::MoveRelative(PixelPoint d)
{
  MoveAbsolute(GetPosition() + rotate.DoRelative(d));
}

Event
MergeMouse::Generate()
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
