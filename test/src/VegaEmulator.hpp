/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "DeviceEmulator.hpp"
#include "Device/Util/LineSplitter.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "util/Macros.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"

#include <string>
#include <map>
#include <stdio.h>

class VegaEmulator : public Emulator, PortLineSplitter {
  std::map<std::string, std::string, std::less<>> settings;

public:
  VegaEmulator() {
    handler = this;
  }

private:
  void PDVSC_S(NMEAInputLine &line) {
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

  void PDVSC_R(NMEAInputLine &line) {
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

  void PDVSC(NMEAInputLine &line) {
    const auto command = line.ReadView();
    if (command == "S"sv)
      PDVSC_S(line);
    else if (command == "R"sv)
      PDVSC_R(line);
  }

protected:
  bool DataReceived(std::span<const std::byte> s) noexcept override {
    fwrite(s.data(), 1, s.size(), stdout);
    return PortLineSplitter::DataReceived(s);
  }

  bool LineReceived(const char *_line) noexcept override {
    if (!VerifyNMEAChecksum(_line))
      return true;

    NMEAInputLine line(_line);
    if (line.ReadCompare("$PDVSC"))
      PDVSC(line);

    return true;
  }
};
