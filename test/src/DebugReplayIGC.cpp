// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplayIGC.hpp"
#include "io/FileLineReader.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "Units/System.hpp"
#include "system/Path.hpp"

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

  basic.clock = basic.time = TimeStamp{fix.time.DurationSinceMidnight()};
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
