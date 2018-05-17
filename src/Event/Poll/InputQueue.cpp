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

InputEventQueue::InputEventQueue(boost::asio::io_service &io_service,
                                 EventQueue &queue)
  :
#ifdef KOBO
   keyboard(io_service, queue, merge_mouse),
   mouse(io_service, queue, merge_mouse)
#else
   libinput_handler(io_service, queue)
#endif
{
#ifdef KOBO
  /* power button */
  keyboard.Open("/dev/input/event0");

  /* Kobo touch screen */
  mouse.Open("/dev/input/event1");
#else
  libinput_handler.Open();
#endif
}

InputEventQueue::~InputEventQueue()
{
}

bool
InputEventQueue::Generate(Event &event)
{
#ifdef KOBO
  event = merge_mouse.Generate();
  if (event.type != Event::Type::NOP)
    return true;
#endif

  return false;
}
