// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEAWriter.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Checksum.hpp"

#include <cassert>

#include <stdio.h>
#include <string.h>

void
PortWriteNMEA(Port &port, const char *line, OperationEnvironment &env)
{
  assert(line != nullptr);

  /* reasonable hard-coded timeout; do we need to make this a
     parameter? */
  static constexpr auto timeout = std::chrono::seconds(1);

  port.Write('$');
  port.FullWrite(line, env, timeout);

  char checksum[16];
  sprintf(checksum, "*%02X\r\n", NMEAChecksum(line));
  port.FullWrite(checksum, env, timeout);
}
