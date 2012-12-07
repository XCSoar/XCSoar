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

#include "DebugReplay.hpp"
#include "OS/Args.hpp"
#include "IO/FileLineReader.hpp"
#include "OS/PathName.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Port/NullPort.hpp"
#include "Device/Parser.hpp"
#include "Profile/DeviceConfig.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCExtensions.hpp"
#include "IGC/IGCFix.hpp"
#include "Units/System.hpp"
#include "ComputerSettings.hpp"

#include <memory>

static DeviceConfig config;
static NullPort port;

DebugReplay::DebugReplay(NLineReader *_reader)
  :reader(_reader), glide_polar(fixed(1))
{
  raw_basic.Reset();
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
  return reader->GetSize();
}

long
DebugReplay::Tell() const
{
  return reader->Tell();
}

void
DebugReplay::Compute()
{
  computed_basic.Reset();
  (NMEAInfo &)computed_basic = raw_basic;

  FeaturesSettings features;
  features.nav_baro_altitude_enabled = true;
  computer.Fill(computed_basic, AtmosphericPressure::Standard(), features);

  computer.Compute(computed_basic, last_basic, last_basic, calculated);
  flying_computer.Compute(glide_polar.GetVTakeoff(),
                          computed_basic, calculated,
                          calculated.flight);
}

class DebugReplayNMEA : public DebugReplay {
  std::unique_ptr<Device> device;

  NMEAParser parser;

public:
  DebugReplayNMEA(NLineReader *reader, const DeviceRegister *driver);

  virtual bool Next();
};

DebugReplayNMEA::DebugReplayNMEA(NLineReader *_reader,
                                 const DeviceRegister *driver)
  :DebugReplay(_reader),
   device(driver->CreateOnPort != NULL
          ? driver->CreateOnPort(config, port)
          : NULL)
{
}

bool
DebugReplayNMEA::Next()
{
  last_basic = computed_basic;

  const char *line;
  while ((line = reader->ReadLine()) != NULL) {
    if (raw_basic.time_available)
      raw_basic.clock = raw_basic.time;
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

class DebugReplayIGC : public DebugReplay {
  IGCExtensions extensions;

  unsigned day;

public:
  DebugReplayIGC(NLineReader *reader)
    :DebugReplay(reader), day(0) {
    extensions.clear();
  }

  virtual bool Next();

protected:
  void CopyFromFix(const IGCFix &fix);
};

bool
DebugReplayIGC::Next()
{
  last_basic = computed_basic;

  const char *line;
  while ((line = reader->ReadLine()) != NULL) {
    if (line[0] == 'B') {
      IGCFix fix;
      if (IGCParseFix(line, extensions, fix) && fix.gps_valid) {
        CopyFromFix(fix);

        Compute();
        return true;
      }
    } else if (line[0] == 'H') {
      BrokenDate date;
      if (memcmp(line, "HFDTE", 5) == 0 &&
          IGCParseDateRecord(line, date)) {
        (BrokenDate &)raw_basic.date_time_utc = date;
        raw_basic.date_available = true;
      }
    } else if (line[0] == 'I') {
      IGCParseExtensions(line, extensions);
    }
  }

  if (computed_basic.time_available)
    flying_computer.Finish(calculated.flight, computed_basic.time);

  return false;
}

void
DebugReplayIGC::CopyFromFix(const IGCFix &fix)
{
  NMEAInfo &basic = raw_basic;

  if (basic.time_available && basic.date_time_utc.hour >= 23 &&
      fix.time.hour == 0) {
    /* midnight roll-over */
    ++day;
    raw_basic.date_time_utc.IncrementDay();
  }

  basic.clock = basic.time =
    fixed(day * 24 * 3600 + fix.time.GetSecondOfDay());
  basic.time_available.Update(basic.clock);
  basic.date_time_utc.hour = fix.time.hour;
  basic.date_time_utc.minute = fix.time.minute;
  basic.date_time_utc.second = fix.time.second;
  basic.alive.Update(basic.clock);
  basic.location = fix.location;
  basic.location_available.Update(basic.clock);

  if (fix.gps_altitude != 0) {
    basic.gps_altitude = fixed(fix.gps_altitude);
    basic.gps_altitude_available.Update(basic.clock);
  }

  if (fix.pressure_altitude != 0) {
    basic.pressure_altitude = basic.baro_altitude = fixed(fix.pressure_altitude);
    basic.pressure_altitude_available.Update(basic.clock);
    basic.baro_altitude_available.Update(basic.clock);
  }

  if (fix.enl >= 0) {
    basic.engine_noise_level = fix.enl;
    basic.engine_noise_level_available.Update(basic.clock);
  }

  if (fix.trt >= 0) {
    basic.track = Angle::Degrees(fixed(fix.trt));
    basic.track_available.Update(basic.clock);
  }

  if (fix.gsp >= 0) {
    basic.ground_speed = Units::ToSysUnit(fixed(fix.gsp),
                                          Unit::KILOMETER_PER_HOUR);
    basic.ground_speed_available.Update(basic.clock);
  }

  if (fix.ias >= 0) {
    fixed ias = Units::ToSysUnit(fixed(fix.ias), Unit::KILOMETER_PER_HOUR);
    if (fix.tas >= 0)
      basic.ProvideBothAirspeeds(ias,
                                 Units::ToSysUnit(fixed(fix.tas),
                                                  Unit::KILOMETER_PER_HOUR));
    else
      basic.ProvideIndicatedAirspeedWithAltitude(ias, basic.pressure_altitude);
  } else if (fix.tas >= 0)
    basic.ProvideTrueAirspeed(Units::ToSysUnit(fixed(fix.tas),
                                               Unit::KILOMETER_PER_HOUR));

  if (fix.siu >= 0) {
    basic.gps.satellites_used = fix.siu;
    basic.gps.satellites_used_available.Update(basic.clock);
  }
}

DebugReplay *
CreateDebugReplay(Args &args)
{
  if (!args.IsEmpty() && MatchesExtension(args.PeekNext(), ".igc")) {
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
