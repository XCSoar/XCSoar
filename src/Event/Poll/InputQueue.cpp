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

#include "InputQueue.hpp"
#include "../Shared/Event.hpp"
#include "DisplayOrientation.hpp"

InputEventQueue::InputEventQueue(IOLoop &io_loop, EventQueue &queue)
  :
#ifdef USE_LIBINPUT
   libinput_handler(io_loop, queue)
#else /* !USE_LIBINPUT */
#ifdef KOBO
   keyboard(io_loop, queue, merge_mouse),
   mouse(io_loop, queue, merge_mouse)
#elif defined(USE_LINUX_INPUT)
   all_input(io_loop, queue, merge_mouse)
#else
   keyboard(queue, io_loop),
   mouse(io_loop, merge_mouse)
#endif
#endif /* !USE_LIBINPUT */
{
#ifdef USE_LIBINPUT
  libinput_handler.Open();
#else /* !USE_LIBINPUT */
#ifdef KOBO
  /* power button */
  keyboard.Open("/dev/input/event0");

  /* Kobo touch screen */
  mouse.Open("/dev/input/event1");
#elif defined(USE_LINUX_INPUT)
  all_input.Open();
#else
  mouse.Open();
#endif
#endif /* !USE_LIBINPUT */
}

InputEventQueue::~InputEventQueue()
{
}

#ifndef USE_LIBINPUT

void
InputEventQueue::SetMouseRotation(DisplayOrientation orientation)
{
  switch (orientation) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::PORTRAIT:
    SetMouseRotation(true, true, false);
    break;

  case DisplayOrientation::LANDSCAPE:
    SetMouseRotation(false, false, false);
    break;

  case DisplayOrientation::REVERSE_PORTRAIT:
    SetMouseRotation(true, false, true);
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    SetMouseRotation(false, true, true);
    break;
  }
}

#endif

bool
InputEventQueue::Generate(Event &event)
{
#ifndef USE_LIBINPUT
  event = merge_mouse.Generate();
  if (event.type != Event::Type::NOP)
    return true;
#endif

  return false;
}
