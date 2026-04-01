// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEAWriter.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Checksum.hpp"

#include <cassert>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

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

void
PortWriteNMEAFormat(Port &port, OperationEnvironment &env,
                    const char *format, ...)
{
  assert(format != nullptr);

  char line[256];

  va_list ap;
  va_start(ap, format);
  const int n = vsnprintf(line, sizeof(line), format, ap);
  va_end(ap);

  if (n <= 0)
    return;

  if ((size_t)n >= sizeof(line)) {
    /* truncated; refuse to send incomplete payload */
    return;
  }

  PortWriteNMEA(port, line, env);
}
