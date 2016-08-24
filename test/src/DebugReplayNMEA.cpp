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

#include "DebugReplayNMEA.hpp"
#include "IO/FileLineReader.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Port/NullPort.hpp"
#include "Device/Parser.hpp"
#include "Device/Config.hpp"

static DeviceConfig config;
static NullPort port;

DebugReplayNMEA::DebugReplayNMEA(FileLineReaderA *_reader,
                                 const DeviceRegister *driver)
  :DebugReplayFile(_reader),
   device(driver->CreateOnPort != NULL
          ? driver->CreateOnPort(config, port)
          : NULL)
{
  clock.Reset();
}

DebugReplay*
DebugReplayNMEA::Create(Path input_file, const tstring &driver_name)
{
  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name.c_str());
    return nullptr;
  }

  FileLineReaderA *reader = new FileLineReaderA(input_file);
  return new DebugReplayNMEA(reader, driver);
}


bool
DebugReplayNMEA::Next()
{
  last_basic = computed_basic;

  const char *line;
  while ((line = reader->ReadLine()) != NULL) {
    raw_basic.clock = clock.NextClock(raw_basic.time_available
                                      ? raw_basic.time
                                      : -1.);

    if (!device || !device->ParseNMEA(line, raw_basic))
      parser.ParseLine(line, raw_basic);

    if (raw_basic.location_available != last_basic.location_available) {
      Compute();
      return true;
    }
  }

  if (computed_basic.time_available)
    flying_computer.Finish(calculated.flight, computed_basic.time);

  return false;
}
