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

#include "Input.hpp"
#include "Event/Shared/Event.hpp"
#include "IO/Async/IOLoop.hpp"

#include <linux/input.h>

bool
LinuxInputDevice::Open(const char *path)
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
LinuxInputDevice::Close()
{
  if (!IsOpen())
    return;

  io_loop.Remove(fd.Get());
  fd.Close();
}

void
LinuxInputDevice::Read()
{
  struct input_event buffer[64];
  const ssize_t nbytes = fd.Read(buffer, sizeof(buffer));
  unsigned n = size_t(nbytes) / sizeof(buffer[0]);

  for (unsigned i = 0; i < n; ++i) {
    const struct input_event &e = buffer[i];

    switch (e.type) {
    case EV_SYN:
      // TODO
      break;

    case EV_KEY:
      if (e.code == BTN_TOUCH) {
        bool new_down = e.value;
        if (new_down != down) {
          down = new_down;
          if (new_down)
            pressed = true;
          else
            released = true;
        }
      }

      break;

    case EV_ABS:
      moved = true;
      if (e.code == 0)
        y = e.value;
      else if (e.code == 1)
        // TODO: hard-coded number
        x = 600 - e.value;
      break;
    }
  }
}

Event
LinuxInputDevice::Generate()
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
LinuxInputDevice::OnFileEvent(int fd, unsigned mask)
{
  Read();

  return true;
}
