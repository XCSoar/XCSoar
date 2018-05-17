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

#include "DebugReplayIGC.hpp"
#include "IO/FileLineReader.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "Units/System.hpp"
#include "OS/Path.hpp"

DebugReplay*
DebugReplayIGC::Create(Path input_file)
{
  FileLineReaderA *reader = new FileLineReaderA(input_file);
  return new DebugReplayIGC(reader);
}

bool
DebugReplayIGC::Next()
{
  last_basic = computed_basic;

  const char *line;
  while ((line = reader->ReadLine()) != NULL) {
    if (line[0] == 'B') {
      IGCFix fix;
      if (IGCParseFix(line, extensions, fix)) {
        CopyFromFix(fix);

        Compute();
        return true;
      }
    } else if (line[0] == 'H') {
      BrokenDate date;
      if (memcmp(line, "HFDTE", 5) == 0 &&
          IGCParseDateRecord(line, date)) {
        (BrokenDate &)raw_basic.date_time_utc = date;
        raw_basic.time_available.Clear();
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
    raw_basic.date_time_utc.IncrementDay();
  }

  basic.clock = basic.time = fix.time.GetSecondOfDay();
  basic.time_available.Update(basic.clock);
  basic.date_time_utc.hour = fix.time.hour;
  basic.date_time_utc.minute = fix.time.minute;
  basic.date_time_utc.second = fix.time.second;
  basic.alive.Update(basic.clock);
  basic.location = fix.location;

  if (fix.gps_valid) {
    basic.location_available.Update(basic.clock);
    basic.gps_altitude = fix.gps_altitude;
    basic.gps_altitude_available.Update(basic.clock);
  } else {
    basic.location_available.Clear();
    basic.gps_altitude_available.Clear();
  }

  if (fix.pressure_altitude != 0) {
    basic.pressure_altitude = fix.pressure_altitude;
    basic.pressure_altitude_available.Update(basic.clock);
  }

  if (fix.enl >= 0) {
    basic.engine_noise_level = fix.enl;
    basic.engine_noise_level_available.Update(basic.clock);
  }

  if (fix.trt >= 0) {
    basic.track = Angle::Degrees(fix.trt);
    basic.track_available.Update(basic.clock);
  }

  if (fix.gsp >= 0) {
    basic.ground_speed = Units::ToSysUnit(fix.gsp, Unit::KILOMETER_PER_HOUR);
    basic.ground_speed_available.Update(basic.clock);
  }

  if (fix.ias >= 0) {
    auto ias = Units::ToSysUnit(fix.ias, Unit::KILOMETER_PER_HOUR);
    if (fix.tas >= 0)
      basic.ProvideBothAirspeeds(ias,
                                 Units::ToSysUnit(fix.tas,
                                                  Unit::KILOMETER_PER_HOUR));
    else
      basic.ProvideIndicatedAirspeedWithAltitude(ias, basic.pressure_altitude);
  } else if (fix.tas >= 0)
    basic.ProvideTrueAirspeed(Units::ToSysUnit(fix.tas,
                                               Unit::KILOMETER_PER_HOUR));

  if (fix.siu >= 0) {
    basic.gps.satellites_used = fix.siu;
    basic.gps.satellites_used_available.Update(basic.clock);
  }
}
