// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "LogFile.hpp"

#include <string.h>

using std::string_view_literals::operator""sv;

bool
FlarmDevice::ParsePFLAC(NMEAInputLine &line)
{
  const auto responsetype = line.ReadView();

  const auto name = line.ReadView();

  if (name == "ERROR"sv) {
    const auto error_info = line.Rest();
    if (!error_info.empty()) {
      LogFormat("FLARM: PFLAC error response (type: %.*s): %.*s",
                (int)responsetype.size(), responsetype.data(),
                (int)error_info.size(), error_info.data());
    } else {
      LogFormat("FLARM: PFLAC error response (type: %.*s)",
                (int)responsetype.size(), responsetype.data());
    }
    return true;
  }

  const auto value = line.Rest();

  const std::lock_guard<Mutex> lock(settings);
  settings.Set(std::string{name}, value);

  return true;
}

bool
FlarmDevice::ParseNMEA(const char *_line, [[maybe_unused]] NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line)) {
    return false;
  }

  NMEAInputLine line(_line);

  const auto type = line.ReadView();
  if (type == "$PFLAC"sv) {
    return ParsePFLAC(line);
  }

  return false;
}
