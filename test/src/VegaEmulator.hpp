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

#ifndef XCSOAR_VEGA_EMULATOR_HPP
#define XCSOAR_VEGA_EMULATOR_HPP

#include "DeviceEmulator.hpp"
#include "Device/Util/LineSplitter.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Util/Macros.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"

#include <string>
#include <map>
#include <stdio.h>
#include <string.h>

class VegaEmulator : public Emulator, PortLineSplitter {
  std::map<std::string, std::string> settings;

public:
  VegaEmulator() {
    handler = this;
  }

private:
  void PDVSC_S(NMEAInputLine &line) {
    char name[64], value[256];
    line.Read(name, ARRAY_SIZE(name));
    line.Read(value, ARRAY_SIZE(value));

    settings[name] = value;

    ConsoleOperationEnvironment env;

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PDVSC,A,%s,%s", name, value);
    PortWriteNMEA(*port, buffer, env);
  }

  void PDVSC_R(NMEAInputLine &line) {
    char name[64];
    line.Read(name, ARRAY_SIZE(name));

    auto i = settings.find(name);
    if (i == settings.end())
      return;

    const char *value = i->second.c_str();

    ConsoleOperationEnvironment env;

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PDVSC,A,%s,%s", name, value);
    PortWriteNMEA(*port, buffer, env);
  }

  void PDVSC(NMEAInputLine &line) {
    char command[4];
    line.Read(command, ARRAY_SIZE(command));

    if (strcmp(command, "S") == 0)
      PDVSC_S(line);
    else if (strcmp(command, "R") == 0)
      PDVSC_R(line);
  }

protected:
  virtual void DataReceived(const void *data, size_t length) {
    fwrite(data, 1, length, stdout);
    PortLineSplitter::DataReceived(data, length);
  }

  virtual void LineReceived(const char *_line) {
    if (!VerifyNMEAChecksum(_line))
      return;

    NMEAInputLine line(_line);
    if (line.ReadCompare("$PDVSC"))
      PDVSC(line);
  }
};

#endif
