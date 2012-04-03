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
#include "Device/Internal.hpp"
#include "TimeoutClock.hpp"

#include <assert.h>

bool
FlarmDevice::TextMode(OperationEnvironment &env)
{
  /* the "text" mode is the same as NMEA mode, only the Port thread is
     stopped */

  if (!EnableNMEA(env))
    return false;

  port.StopRxThread();
  mode = Mode::TEXT;
  return true;
}

bool
FlarmDevice::Send(const char *sentence, OperationEnvironment &env)
{
  assert(sentence != NULL);

  /* From the FLARM data port specification: "All sentences must [...]
     end with [...] two checksum characters [...].  [...] these
     characters [...] must be provided in sentences to FLARM and are
     part of the answers given by FLARM." */
  return PortWriteNMEA(port, sentence, env);
}

bool
FlarmDevice::Receive(const char *prefix, char *buffer, size_t length,
                     OperationEnvironment &env, unsigned timeout_ms)
{
  assert(prefix != NULL);

  TimeoutClock timeout(timeout_ms);

  if (!port.ExpectString(prefix, env, timeout_ms))
    return false;

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    int remaining = timeout.GetRemainingSigned();
    if (remaining < 0)
      /* timeout */
      return false;

    if (port.WaitRead(env, remaining) != Port::WaitResult::READY)
      return false;

    // Read single character from port
    int c = port.GetChar();
    if (c == -1)
      return false;

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
