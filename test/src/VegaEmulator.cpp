// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VegaEmulator.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "util/Macros.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"

#include <stdio.h>

using std::string_view_literals::operator""sv;

inline void
VegaEmulator::PDVSC_S(NMEAInputLine &line) noexcept
{
  const auto name = line.ReadView();
  const auto value = line.ReadView();

  settings[std::string{name}] = value;

  ConsoleOperationEnvironment env;

  char buffer[512];
  snprintf(buffer, ARRAY_SIZE(buffer), "PDVSC,A,%.*s,%.*s",
           (int)name.size(), name.data(),
           (int)value.size(), value.data());
  PortWriteNMEA(*port, buffer, env);
}

inline void
VegaEmulator::PDVSC_R(NMEAInputLine &line) noexcept
{
  const auto name = line.ReadView();

  auto i = settings.find(name);
  if (i == settings.end())
    return;

  const char *value = i->second.c_str();

  ConsoleOperationEnvironment env;

  char buffer[512];
  snprintf(buffer, ARRAY_SIZE(buffer), "PDVSC,A,%.*s,%s",
           (int)name.size(), name.data(),
           value);
  PortWriteNMEA(*port, buffer, env);
}

inline void
VegaEmulator::PDVSC(NMEAInputLine &line) noexcept
{
  const auto command = line.ReadView();
  if (command == "S"sv)
    PDVSC_S(line);
  else if (command == "R"sv)
    PDVSC_R(line);
}

bool
VegaEmulator::DataReceived(std::span<const std::byte> s) noexcept
{
  fwrite(s.data(), 1, s.size(), stdout);
  return PortLineSplitter::DataReceived(s);
}

bool
VegaEmulator::LineReceived(const char *_line) noexcept
{
  if (!VerifyNMEAChecksum(_line))
    return true;

  NMEAInputLine line(_line);
  if (line.ReadCompare("$PDVSC"))
    PDVSC(line);

  return true;
}
