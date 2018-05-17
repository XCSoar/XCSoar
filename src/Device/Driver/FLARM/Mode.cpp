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

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"

bool
FlarmDevice::EnableNMEA(OperationEnvironment &env)
{
  switch (mode) {
  case Mode::UNKNOWN:
    /* device could be in binary mode, we don't know, but this is the
       best we can do: */
    if (!BinaryReset(env, 500))
      return false;

    mode = Mode::NMEA;

    /* request self-test results and version information from FLARM */
    Send("PFLAE,R", env);
    Send("PFLAV,R", env);
    return true;

  case Mode::NMEA:
    return true;

  case Mode::TEXT:
    /* no real difference between NMEA and TEXT; in mode==TEXT, the
       Port thread is stopped, but the caller is responsible for
       restarting it, which means there's nothing to do for us */
    mode = Mode::NMEA;
    return true;

  case Mode::BINARY:
    if (!BinaryReset(env, 500)) {
      mode = Mode::UNKNOWN;
      return false;
    }

    mode = Mode::NMEA;
    return true;
  }

  gcc_unreachable();
  assert(false);
  return false;
}

bool
FlarmDevice::BinaryMode(OperationEnvironment &env)
{
  if (mode == Mode::BINARY)
    return true;

  port.StopRxThread();

  // "Binary mode is engaged by sending the text command "$PFLAX"
  // (including a newline character) to Flarm."
  if (!Send("PFLAX", env))
    return false;

  // Remember that we should now be in binary mode (for further assert() calls)
  mode = Mode::BINARY;

  // "After switching, connection should again be checked by issuing a ping."
  // Testing has revealed that switching the protocol takes a certain amount
  // of time (around 1.5 sec). Due to that it is recommended to issue new pings
  // for a certain time until the ping is ACKed properly or a timeout occurs.
  for (unsigned i = 0; i < 10; ++i) {
    if (env.IsCancelled()) {
      BinaryReset(env, 200);
      mode = Mode::UNKNOWN;
      return false;
    }

    if (BinaryPing(env, 500))
      // We are now in binary mode and have verified that with a binary ping
      return true;
  }

  // Apparently the switch to binary mode didn't work
  mode = Mode::UNKNOWN;
  return false;
}
