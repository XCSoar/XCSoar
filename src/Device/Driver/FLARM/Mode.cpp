// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "Device/Port/Port.hpp"
#include "LogFile.hpp"
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

    /* request self-test results, version and radio id from FLARM */
    Send("PFLAE,R", env);
    Send("PFLAV,R", env);
    Send("PFLAC,R,RADIOID", env);
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

  /* especially on Bluetooth LE ports, the connection may not be
     established yet (e.g. iOS is still connecting to the
     peripheral, which has no timeout); wait for it (cancellable)
     instead of wasting the ping attempts below on a dead link, like
     the LX Nano driver does */
  if (!port.WaitConnected(env)) {
    LogFormat("FLARM: port not connected, cannot enter binary mode");
    return false;
  }

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
    /* give slow links (small ATT MTU, long connection interval) some
       more time after the first attempts */
    const auto timeout = std::chrono::milliseconds(i < 5 ? 500 : 1000);

    if (BinaryPing(env, timeout)) {
      // We are now in binary mode and have verified that with a binary ping

      // Remember that we should now be in binary mode (for further assert() calls)
      was_binary = true;
      mode = Mode::BINARY;

#ifndef NDEBUG
      if (i > 0)
        LogFormat("FLARM: binary mode established after %u pings", i + 1);
#endif

      return true;
    }
  }

  // Apparently the switch to binary mode didn't work
  LogFormat("FLARM: no answer to binary ping, cannot enter binary mode");
  return false;
}
