/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_FLARM_EMULATOR_HPP
#define XCSOAR_FLARM_EMULATOR_HPP

#include "DeviceEmulator.hpp"
#include "Device/Port/LineHandler.hpp"
#include "Device/Internal.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Util/Macros.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"

#include <string>
#include <map>
#include <stdio.h>
#include <string.h>

class FLARMEmulator : public Emulator, PortLineHandler {
  std::map<std::string, std::string> settings;

public:
  FLARMEmulator() {
    handler = this;
  }

private:
  void PFLAC_S(NMEAInputLine &line) {
    char name[64];
    line.read(name, ARRAY_SIZE(name));
    const char *value = line.rest();

    settings[name] = value;

    ConsoleOperationEnvironment env;

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%s,%s", name, value);
    PortWriteNMEA(*port, buffer, env);
  }

  void PFLAC_R(NMEAInputLine &line) {
    char name[64];
    line.read(name, ARRAY_SIZE(name));

    auto i = settings.find(name);
    if (i == settings.end())
      return;

    ConsoleOperationEnvironment env;

    const char *value = i->second.c_str();

    char buffer[512];
    snprintf(buffer, ARRAY_SIZE(buffer), "PFLAC,A,%s,%s", name, value);
    PortWriteNMEA(*port, buffer, env);
  }

  void PFLAC(NMEAInputLine &line) {
    char command[4];
    line.read(command, ARRAY_SIZE(command));

    if (strcmp(command, "S") == 0)
      PFLAC_S(line);
    else if (strcmp(command, "R") == 0)
      PFLAC_R(line);
  }

protected:
  virtual void DataReceived(const void *data, size_t length) {
    fwrite(data, 1, length, stdout);
    PortLineHandler::DataReceived(data, length);
  }

  virtual void LineReceived(const char *_line) {
    const char *dollar = strchr(_line, '$');
    if (dollar != NULL)
      _line = dollar;

    if (!VerifyNMEAChecksum(_line))
      return;

    NMEAInputLine line(_line);
    if (line.read_compare("$PFLAC"))
      PFLAC(line);
  }
};

#endif
