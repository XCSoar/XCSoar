/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "DebugReplay.hpp"
#include "Args.hpp"
#include "IO/FileLineReader.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/NullPort.hpp"
#include "Device/Parser.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Replay/IGCParser.hpp"

static DeviceConfig config;
static NullPort port;

DebugReplay::DebugReplay(NLineReader *_reader)
  :reader(_reader), glide_polar(fixed_one)
{
  settings_computer.SetDefaults();
  basic.Reset();
  calculated.Reset();

  flying_computer.Reset();
}

DebugReplay::~DebugReplay()
{
  delete reader;
}

long
DebugReplay::Size() const
{
  return reader->size();
}

long
DebugReplay::Tell() const
{
  return reader->tell();
}

void
DebugReplay::Compute()
{
  computer.Fill(basic, settings_computer);
  computer.Compute(basic, last_basic, calculated, settings_computer);
  flying_computer.Compute(glide_polar, basic, last_basic, calculated,
                          calculated.flight);
}

class DebugReplayNMEA : public DebugReplay {
  Device *device;

  NMEAParser parser;

public:
  DebugReplayNMEA(NLineReader *reader, const DeviceRegister *driver);
  ~DebugReplayNMEA();

  virtual bool Next();
};

DebugReplayNMEA::DebugReplayNMEA(NLineReader *_reader,
                                 const DeviceRegister *driver)
  :DebugReplay(_reader),
   device(driver->CreateOnPort != NULL
          ? driver->CreateOnPort(config, &port)
          : NULL)
{
}

DebugReplayNMEA::~DebugReplayNMEA()
{
  delete device;
}

bool
DebugReplayNMEA::Next()
{
  last_basic = basic;
  last_calculated = calculated;

  const char *line;
  while ((line = reader->read()) != NULL) {
    if (basic.time_available)
      basic.clock = basic.time;
    if (device == NULL || !device->ParseNMEA(line, basic))
      parser.ParseNMEAString_Internal(line, basic);

    if (basic.location_available != last_basic.location_available) {
      Compute();
      return true;
    }
  }

  return false;
}

class DebugReplayIGC : public DebugReplay {
public:
  DebugReplayIGC(NLineReader *reader)
    :DebugReplay(reader) {}

  virtual bool Next();

protected:
  void CopyFromFix(const IGCFix &fix);
};

bool
DebugReplayIGC::Next()
{
  last_basic = basic;
  last_calculated = calculated;

  const char *line;
  while ((line = reader->read()) != NULL) {
    if (line[0] == 'B') {
      IGCFix fix;
      if (IGCParseFix(line, fix)) {
        CopyFromFix(fix);

        Compute();
        return true;
      }
    }
  }

  return false;
}

void
DebugReplayIGC::CopyFromFix(const IGCFix &fix)
{
  basic.clock = basic.time = fixed(fix.time.GetSecondOfDay());
  basic.time_available.Update(basic.clock);
  basic.date_time_utc.year = 2011;
  basic.date_time_utc.month = 6;
  basic.date_time_utc.day = 5;
  basic.date_time_utc.hour = fix.time.hour;
  basic.date_time_utc.minute = fix.time.minute;
  basic.date_time_utc.second = fix.time.second;
  basic.connected.Update(basic.clock);
  basic.location = fix.location;
  basic.location_available.Update(basic.clock);
  basic.gps_altitude = fix.gps_altitude;
  basic.gps_altitude_available.Update(basic.clock);
  basic.pressure_altitude = basic.baro_altitude = fix.pressure_altitude;
  basic.pressure_altitude_available.Update(basic.clock);
  basic.baro_altitude_available.Update(basic.clock);
}

DebugReplay *
CreateDebugReplay(Args &args)
{
  if (!args.IsEmpty() && strstr(args.PeekNext(), ".igc") != NULL) {
    const char *input_file = args.ExpectNext();

    FileLineReaderA *reader = new FileLineReaderA(input_file);
    if (reader->error()) {
      delete reader;
      fprintf(stderr, "Failed to open %s\n", input_file);
      return NULL;
    }

    return new DebugReplayIGC(reader);
  }

  const tstring driver_name = args.ExpectNextT();

  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name.c_str());
    return NULL;
  }

  const char *input_file = args.ExpectNext();

  FileLineReaderA *reader = new FileLineReaderA(input_file);
  if (reader->error()) {
    delete reader;
    fprintf(stderr, "Failed to open %s\n", input_file);
    return NULL;
  }

  return new DebugReplayNMEA(reader, driver);
}
