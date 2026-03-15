// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"

bool
FlarmDevice::EnableNMEA(OperationEnvironment &env)
{
  switch (mode) {
  case Mode::UNKNOWN:
    /* Use tracked binary state to avoid unnecessary EXIT frames. */
    if (was_binary)
      BinaryReset(env, std::chrono::milliseconds(500));
    was_binary = false;
    mode = Mode::NMEA;

    /* request self-test results and version information from FLARM */
    Send("PFLAE,R", env);
    Send("PFLAV,R", env);
    return true;

  case Mode::NMEA:
    was_binary = false;
    return true;

  case Mode::TEXT:
    /* no real difference between NMEA and TEXT; in mode==TEXT, the
       Port thread is stopped, but the caller is responsible for
       restarting it, which means there's nothing to do for us */
    was_binary = false;
    mode = Mode::NMEA;
    return true;

  case Mode::BINARY:
    was_binary = true;
    mode = Mode::UNKNOWN;
    BinaryReset(env, std::chrono::milliseconds(500));
    was_binary = false;
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
  Send("PFLAX", env);

  mode = Mode::UNKNOWN;

  // "After switching, connection should again be checked by issuing a ping."
  // Testing has revealed that switching the protocol takes a certain amount
  // of time (around 1.5 sec). Due to that it is recommended to issue new pings
  // for a certain time until the ping is ACKed properly or a timeout occurs.
  for (unsigned i = 0; i < 10; ++i) {
    if (BinaryPing(env, std::chrono::milliseconds(500))) {
      // We are now in binary mode and have verified that with a binary ping

      // Remember that we should now be in binary mode (for further assert() calls)
      was_binary = true;
      mode = Mode::BINARY;
      return true;
    }
  }

  // Apparently the switch to binary mode didn't work
  return false;
}
