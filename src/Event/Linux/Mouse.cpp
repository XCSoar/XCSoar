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

#include "Mouse.hpp"
#include "Event/Shared/Event.hpp"
#include "IO/Async/IOLoop.hpp"

void
LinuxMouse::SetScreenSize(unsigned width, unsigned height)
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

bool
LinuxMouse::Open(const char *path)
{
  if (!fd.OpenReadOnly(path))
    return false;

  fd.SetNonBlocking();
  io_loop.Add(fd.Get(), io_loop.READ, *this);

  down = false;
  moved = pressed = released = false;
  return true;
}

void
LinuxMouse::Close()
{
  if (!IsOpen())
    return;

  io_loop.Remove(fd.Get());
  fd.Close();
}

void
LinuxMouse::Read()
{
  bool old_down = down;
  int8_t mb[3];
  while (fd.Read(mb, sizeof(mb)) == sizeof(mb)) {
    down = (mb[0] & 0x7) != 0;
    if (down != old_down) {
      if (down)
        pressed = true;
      else
        released = true;
    }

    const int dx = mb[1], dy = mb[2];
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
}

Event
LinuxMouse::Generate()
{
  if (!IsOpen())
    return Event(Event::Type::NOP);

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

bool
LinuxMouse::OnFileEvent(int fd, unsigned mask)
{
  Read();

  return true;
}
