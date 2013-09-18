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

#include "TTYKeyboard.hpp"
#include "Event/Queue.hpp"
#include "Screen/Key.h"
#include "Util/CharUtil.hpp"
#include "IO/Async/IOLoop.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

static struct termios restore_attr;

TTYKeyboard::TTYKeyboard(EventQueue &_queue, IOLoop &_io_loop)
  :queue(_queue), io_loop(_io_loop), input_state(InputState::NONE)
{
  /* make stdin non-blocking */
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

  struct termios attr;
  if (tcgetattr(STDIN_FILENO, &attr) == 0) {
    restore_attr = attr;
    attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                      INLCR | IGNCR | ICRNL | IXON);
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG);
    attr.c_cflag &= ~(CSIZE | PARENB);
    attr.c_cflag |= CS8;
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);
  }

  io_loop.Add(STDIN_FILENO, io_loop.READ, *this);
}

TTYKeyboard::~TTYKeyboard()
{
  io_loop.Remove(STDIN_FILENO);

  tcsetattr(STDIN_FILENO, TCSANOW, &restore_attr);
}

inline void
TTYKeyboard::HandleInputByte(char ch)
{
  switch (ch) {
  case 0x03:
    /* user has pressed Ctrl-C */
    input_state = InputState::NONE;
    queue.Push(Event::CLOSE);
    return;

  case ' ':
    input_state = InputState::NONE;
    queue.PushKeyPress(ch);
    return;

  case 0x0d:
    input_state = InputState::NONE;
    queue.PushKeyPress(KEY_RETURN);
    return;

  case 0x1b:
    if (input_state == InputState::ESCAPE)
      queue.PushKeyPress(KEY_ESCAPE);
    else
      input_state = InputState::ESCAPE;
    return;
  }

  switch (input_state) {
  case InputState::NONE:
    break;

  case InputState::ESCAPE:
    if (ch == '[')
      input_state = InputState::ESCAPE_BRACKET;
    else
      input_state = InputState::NONE;
    return;

  case InputState::ESCAPE_BRACKET:
    input_state = InputState::NONE;
    switch (ch) {
    case 'A':
      queue.PushKeyPress(KEY_UP);
      break;

    case 'B':
      queue.PushKeyPress(KEY_DOWN);
      break;

    case 'C':
      queue.PushKeyPress(KEY_RIGHT);
      break;

    case 'D':
      queue.PushKeyPress(KEY_LEFT);
      break;

    case '[':
      input_state = InputState::ESCAPE_BRACKET2;
      break;

    default:
      if (ch >= '0' && ch <= '9') {
        input_state = InputState::ESCAPE_NUMBER;
        input_number = ch - '0';
      } else
        input_state = InputState::NONE;
    }

    return;

  case InputState::ESCAPE_BRACKET2:
    switch (ch) {
    case 'A':
      queue.PushKeyPress(KEY_F1);
      break;

    case 'B':
      queue.PushKeyPress(KEY_F2);
      break;

    case 'C':
      queue.PushKeyPress(KEY_F3);
      break;

    case 'D':
      queue.PushKeyPress(KEY_F4);
      break;

    case 'E':
      queue.PushKeyPress(KEY_F5);
      break;

    default:
      input_state = InputState::NONE;
    }

    return;

  case InputState::ESCAPE_NUMBER:
    if (IsDigitASCII(ch))
      input_number = input_number * 10 + ch - '0';
    else {
      input_state = InputState::NONE;
      if (ch == '~') {
        if (input_number >= 11 && input_number <= 16)
          queue.PushKeyPress(KEY_F1 + input_number - 11);
        else if (input_number >= 17 && input_number <= 21)
          queue.PushKeyPress(KEY_F6 + input_number - 17);
        else if (input_number >= 23 && input_number <= 24)
          queue.PushKeyPress(KEY_F11 + input_number - 23);
      }
    }

    return;
  }

  if (IsAlphaNumericASCII(ch)) {
      queue.PushKeyPress(ch);
      return;
  }
}

bool
TTYKeyboard::OnFileEvent(int fd, unsigned mask)
{
  char buffer[256];
  const ssize_t nbytes = read(fd, buffer, sizeof(buffer));
  if (nbytes > 0) {
    for (ssize_t i = 0; i < nbytes; ++i)
      HandleInputByte(buffer[i]);
  } else if (nbytes == 0 || errno != EAGAIN) {
    queue.Quit();
    return false;
  }

  return true;
}
