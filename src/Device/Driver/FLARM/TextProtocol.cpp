/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "PeriodClock.hpp"

#include <assert.h>

void
FlarmDevice::Send(const char *sentence)
{
  assert(!in_binary_mode);
  assert(sentence != NULL);

  port.Write('$');
  port.Write(sentence);
  port.Write("\r\n");
}

bool
FlarmDevice::Receive(const char *prefix, char *buffer, size_t length,
                     unsigned timeout_ms)
{
  assert(!in_binary_mode);
  assert(prefix != NULL);

  PeriodClock timeout;
  timeout.update();

  if (!port.ExpectString(prefix, timeout_ms))
    return false;

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.check(timeout_ms))
      return false;

    // Read single character from port
    int c = port.GetChar();

    // On failure try again until timed out
    if (c == -1)
      continue;

    // Break on line break or checksum
    if (c == '*' || c == '\n') {
      *p = '\0';
      break;
    }

    // Skip carriage return
    if (c == '\r')
      continue;

    // Write received character to buffer
    *p = c;
    p++;
  }

  return true;
}
