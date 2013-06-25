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

#ifndef XCSOAR_EVENT_LINUX_TTY_KEYBOARD_HPP
#define XCSOAR_EVENT_LINUX_TTY_KEYBOARD_HPP

#include "IO/Async/FileEventHandler.hpp"

#include <stdint.h>

class EventQueue;
class IOLoop;

/**
 * A keyboard driver reading key presses from the TTY.
 */
class TTYKeyboard final : private FileEventHandler {
  EventQueue &queue;
  IOLoop &io_loop;

  enum class InputState : uint8_t {
    NONE, ESCAPE, ESCAPE_BRACKET, ESCAPE_NUMBER,
  } input_state;

  unsigned input_number;

public:
  TTYKeyboard(EventQueue &queue, IOLoop &io_loop);
  ~TTYKeyboard();

private:
  void HandleInputByte(char ch);

  /* virtual methods from FileEventHandler */
  virtual bool OnFileEvent(int fd, unsigned mask) override;
};

#endif
