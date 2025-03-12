// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"

#include <string.h>

using std::string_view_literals::operator""sv;

bool
FlarmDevice::ParsePFLAC(NMEAInputLine &line)
{
  [[maybe_unused]] const auto responsetype = line.ReadView();

  const auto name = line.ReadView();

  if (name == "ERROR"sv)
    // ignore error responses...
    return true;

  const auto value = line.Rest();

  const std::lock_guard<Mutex> lock(settings);
  settings.Set(std::string{name}, value);

  return true;
}

bool
FlarmDevice::ParseNMEA(const char *_line, [[maybe_unused]] NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  const auto type = line.ReadView();
  if (type == "$PFLAC"sv)
    return ParsePFLAC(line);
  else
    return false;
}
