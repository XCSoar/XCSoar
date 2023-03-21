// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/InputLine.hpp"

#include <string.h>

NMEAInputLine::NMEAInputLine(const char* line) noexcept
  :CSVLine(line)
{
  const char* asterisk = strchr(line, '*');
  if (asterisk != NULL)
    end = asterisk;
}
