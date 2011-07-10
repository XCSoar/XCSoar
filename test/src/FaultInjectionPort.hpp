/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Device/Port.hpp"

#include <algorithm>
#include <assert.h>
#include <stdio.h>

static unsigned inject_port_fault;

class FaultInjectionPort : public Port {
public:
  enum {
    DEFAULT_BAUD_RATE = 1234,
  };

  bool running;
  int timeout;
  unsigned baud_rate;

  FaultInjectionPort(Handler &_handler)
    :Port(_handler), running(true), timeout(0), baud_rate(DEFAULT_BAUD_RATE) {}

  void Flush() {}

  size_t Write(const void *data, size_t length) {
    return length;
  }

  bool StopRxThread() {
    assert(running);
    running = false;
    return true;
  }

  bool StartRxThread() {
    running = true;
    return true;
  }

  int GetChar(void) {
    return EOF;
  }

  bool SetRxTimeout(unsigned _timeout) {
    timeout = _timeout;
    return true;
  }

  unsigned GetBaudrate() const {
    return baud_rate;
  }

  unsigned SetBaudrate(unsigned _baud_rate) {
    unsigned old_value = baud_rate;
    baud_rate = _baud_rate;
    return old_value;
  }

  int Read(void *Buffer, size_t Size) {
    if (inject_port_fault == 0)
      return -1;

    --inject_port_fault;
    char *p = (char *)Buffer;
    std::fill(p, p + Size, ' ');
    return Size;
  }
};

Port::Port(Handler &_handler)
  :handler(_handler) {}

Port::~Port() {}

size_t
Port::Write(const char *s)
{
  return Write(s, strlen(s));
}

bool
Port::FullWrite(const void *buffer, size_t length, unsigned timeout_ms)
{
  return Write(buffer, length) == length;
}

int
Port::GetChar()
{
  unsigned char ch;
  return Read(&ch, sizeof(ch)) == sizeof(ch)
    ? ch
    : EOF;
}

void
Port::FullFlush(unsigned timeout_ms)
{
  Flush();
}

bool
Port::FullRead(void *buffer, size_t length, unsigned timeout_ms)
{
  return Read(buffer, length) == (int)length;
}

bool
Port::ExpectString(const char *token)
{
  if (inject_port_fault == 0)
    return false;

  --inject_port_fault;
  return true;
}
