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

#include "Replay/IgcReplay.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "IO/LineReader.hpp"
#include "NMEA/Info.hpp"
#include "Units/System.hpp"

IgcReplay::IgcReplay(std::unique_ptr<NLineReader> &&_reader)
  :reader(std::move(_reader))
{
  extensions.clear();
}

IgcReplay::~IgcReplay()
{
}

inline bool
IgcReplay::ScanBuffer(const char *buffer, IGCFix &fix, NMEAInfo &basic)
{
  if (IGCParseFix(buffer, extensions, fix) && fix.gps_valid)
    return true;

  BrokenDate date;
  if (IGCParseDateRecord(buffer, date))
    basic.ProvideDate(date);
  else
    IGCParseExtensions(buffer, extensions);

  return false;
}

inline bool
IgcReplay::ReadPoint(IGCFix &fix, NMEAInfo &basic)
{
  char *buffer;

  while ((buffer = reader->ReadLine()) != nullptr) {
    if (ScanBuffer(buffer, fix, basic))
      return true;
  }

  return false;
}

bool
IgcReplay::Update(NMEAInfo &basic)
{
  IGCFix fix;

  while (true) {
    if (!ReadPoint(fix, basic))
      return false;

    if (fix.time.IsPlausible())
      break;
  }

  basic.clock = fix.time.GetSecondOfDay();
  basic.alive.Update(basic.clock);
  basic.ProvideTime(basic.clock);
  basic.location = fix.location;
  basic.location_available.Update(basic.clock);

  if (fix.gps_altitude != 0) {
    basic.gps_altitude = fix.gps_altitude;
    basic.gps_altitude_available.Update(basic.clock);
  } else
    basic.gps_altitude_available.Clear();

  if (fix.pressure_altitude != 0) {
    basic.ProvidePressureAltitude(fix.pressure_altitude);
    basic.ProvideBaroAltitudeTrue(fix.pressure_altitude);
  } else {
    basic.pressure_altitude_available.Clear();
    basic.baro_altitude_available.Clear();
  }

  if (fix.enl >= 0) {
    basic.engine_noise_level = fix.enl;
    basic.engine_noise_level_available.Update(basic.clock);
  } else
    basic.engine_noise_level_available.Clear();

  if (fix.trt >= 0) {
    basic.track = Angle::Degrees(fix.trt);
    basic.track_available.Update(basic.clock);
  } else
    basic.track_available.Clear();

  if (fix.gsp >= 0) {
    basic.ground_speed = Units::ToSysUnit(fix.gsp,
                                          Unit::KILOMETER_PER_HOUR);
    basic.ground_speed_available.Update(basic.clock);
  } else
    basic.ground_speed_available.Clear();

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

  basic.gps.real = false;
  basic.gps.replay = true;
  basic.gps.simulator = false;

  return true;
}
