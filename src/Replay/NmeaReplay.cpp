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

#include "Replay/NmeaReplay.hpp"
#include "IO/LineReader.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Config.hpp"
#include "NMEA/Info.hpp"
#include "Util/StringCompare.hxx"

NmeaReplay::NmeaReplay(std::unique_ptr<NLineReader> &&_reader,
                       const DeviceConfig &config)
  :reader(std::move(_reader)),
   parser(new NMEAParser()),
   device(nullptr)
{
  parser->SetReal(false);

  const struct DeviceRegister *driver = FindDriverByName(config.driver_name);
  assert(driver != nullptr);
  if (driver->CreateOnPort != nullptr) {
    DeviceConfig config;
    config.Clear();
    device = driver->CreateOnPort(config, port);
  }

  clock.Reset();
}

NmeaReplay::~NmeaReplay()
{
  delete device;
  delete parser;
}

bool
NmeaReplay::ParseLine(const char *line, NMEAInfo &data)
{
  data.clock = clock.NextClock(data.time_available ? data.time : -1);

  if ((device != nullptr && device->ParseNMEA(line, data)) ||
      (parser != nullptr && parser->ParseLine(line, data))) {
    data.gps.replay = true;
    data.alive.Update(data.clock);

    return true;
  } else
    return false;
}

bool
NmeaReplay::ReadUntilRMC(NMEAInfo &data)
{
  char *buffer;

  while ((buffer = reader->ReadLine()) != nullptr) {
    ParseLine(buffer, data);

    if ((StringLength(buffer) >= 6 &&
         StringStartsWith(buffer, "$G") &&
         StringStartsWith(buffer + 3, "RMC")) ||
        StringStartsWith(buffer, "$FLYSEN"))
      return true;
  }

  return false;
}

bool
NmeaReplay::Update(NMEAInfo &data)
{
  return ReadUntilRMC(data);
}
