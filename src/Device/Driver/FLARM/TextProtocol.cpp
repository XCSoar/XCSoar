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

#include "TextProtocol.hpp"
#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Time/TimeoutClock.hpp"

#include <assert.h>
#include <string.h>

static constexpr bool
IsForbiddenFlarmChar(unsigned char ch)
{
  return
    /* don't allow ASCII control characters */
    ch < 0x20 ||
    /* don't allow NMEA control characters */
    ch == '$' || ch == '*';
}

char *
CopyCleanFlarmString(char *gcc_restrict dest, const char *gcc_restrict src)
{
  while (true) {
    char ch = *src++;
    if (ch == 0)
      break;

    if (!IsForbiddenFlarmChar(ch))
      *dest++ = ch;
  }

  *dest = 0;
  return dest;
}

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
  assert(sentence != nullptr);

  /* workaround for a Garrecht TRX-1090 firmware bug: start with a new
     line, because the TRX-1090 expects the '$' to be the first
     character, or it won't forward the sentence to the FLARM  */
  if (!port.Write('\n'))
    return false;

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
  assert(prefix != nullptr);

  TimeoutClock timeout(timeout_ms);

  if (!port.ExpectString(prefix, env, timeout_ms))
    return false;

  char *p = (char *)buffer, *end = p + length;
  while (true) {
    size_t nbytes = port.WaitAndRead(p, end - p, env, timeout);
    if (nbytes == 0)
      return false;

    char *q = (char *)memchr(p, '*', nbytes);
    if (q != nullptr) {
      /* stop at checksum */
      *q = 0;
      return true;
    }

    p += nbytes;

    if (p >= end)
      /* line too long */
      return false;
  }
}
