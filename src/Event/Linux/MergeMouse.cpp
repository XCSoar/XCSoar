/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "IO/Async/IOLoop.hpp"

void
MergeMouse::SetScreenSize(unsigned width, unsigned height)
{
  if (width != screen_width) {
    screen_width = width;
    x = screen_width / 2;
  }

  if (height != screen_height) {
    screen_height = height;
    y = screen_height / 2;
  }
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
MergeMouse::MoveRelative(int dx, int dy)
{
  if (screen_width > 0) {
    int new_x = x + dx;
    if (new_x < 0)
      new_x = 0;
    else if (unsigned(new_x) > screen_width)
      new_x = screen_width - 1;

    if (unsigned(new_x) != x) {
      x = new_x;
      moved = true;
    }
  }

  if (screen_height > 0) {
    int new_y = y - dy;
    if (new_y < 0)
      new_y = 0;
    else if (unsigned(new_y) > screen_height)
      new_y = screen_height - 1;

    if (unsigned(new_y) != y) {
      y = new_y;
      moved = true;
    }
  }
}

Event
MergeMouse::Generate()
{
  if (moved) {
    moved = false;
    return Event(Event::MOUSE_MOTION, x, y);
  }

  if (pressed) {
    pressed = false;
    return Event(Event::MOUSE_DOWN, x, y);
  }

  if (released) {
    released = false;
    return Event(Event::MOUSE_UP, x, y);
  }

  return Event(Event::Type::NOP);
}
