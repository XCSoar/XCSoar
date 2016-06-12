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

#include "StaticParser.hpp"
#include "NMEA/InputLine.hpp"
#include "FLARM/Error.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"

void
ParsePFLAE(NMEAInputLine &line, FlarmError &error, double clock)
{
  char type[2];
  line.Read(type, ARRAY_SIZE(type));
  if (!StringIsEqual(type, "A"))
    return;

  error.severity = (FlarmError::Severity)
    line.Read((int)FlarmError::Severity::NO_ERROR);
  error.code = (FlarmError::Code)line.ReadHex(0);

  error.available.Update(clock);
}

void
ParsePFLAV(NMEAInputLine &line, FlarmVersion &version, double clock)
{
  char type[2];
  line.Read(type, ARRAY_SIZE(type));
  if (!StringIsEqual(type, "A"))
    return;

  line.Read(version.hardware_version.buffer(),
            version.hardware_version.capacity());
  version.hardware_version.CleanASCII();

  line.Read(version.software_version.buffer(),
            version.software_version.capacity());
  version.software_version.CleanASCII();

  line.Read(version.obstacle_version.buffer(),
            version.obstacle_version.capacity());
  version.obstacle_version.CleanASCII();

  version.available.Update(clock);
}

void
ParsePFLAU(NMEAInputLine &line, FlarmStatus &flarm, double clock)
{
  flarm.available.Update(clock);

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  flarm.rx = line.Read(0);
  flarm.tx = line.Read(false);
  flarm.gps = (FlarmStatus::GPSStatus)
    line.Read((int)FlarmStatus::GPSStatus::NONE);

  line.Skip();
  flarm.alarm_level = (FlarmTraffic::AlarmType)
    line.Read((int)FlarmTraffic::AlarmType::NONE);
}

/**
 * Parses non-negative floating-point angle value in degrees.
 */
static bool
ReadBearing(NMEAInputLine &line, Angle &value_r)
{
  double value;
  if (!line.ReadChecked(value))
    return false;

  if (value < 0 || value > 360)
    return false;

  value_r = Angle::Degrees(value).AsBearing();
  return true;
}

void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, double clock)
{
  flarm.modified.Update(clock);

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  FlarmTraffic traffic;
  traffic.alarm_level = (FlarmTraffic::AlarmType)
    line.Read((int)FlarmTraffic::AlarmType::NONE);

  double value;
  bool stealth = false;

  if (!line.ReadChecked(value))
    // Relative North is required !
    return;
  traffic.relative_north = value;

  if (!line.ReadChecked(value))
    // Relative East is required !
    return;
  traffic.relative_east = value;

  if (!line.ReadChecked(value))
    // Relative Altitude is required !
    return;
  traffic.relative_altitude = value;

  line.Skip(); /* id type */

  // 5 id, 6 digit hex
  char id_string[16];
  line.Read(id_string, 16);
  traffic.id = FlarmId::Parse(id_string, nullptr);

  Angle track;
  traffic.track_received = ReadBearing(line, track);
  if (!traffic.track_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.track = Angle::Zero();
  } else
    traffic.track = track;

  traffic.turn_rate_received = line.ReadChecked(value);
  if (!traffic.turn_rate_received) {
    // Field is empty in stealth mode
    traffic.turn_rate = 0;
  } else
    traffic.turn_rate = value;

  traffic.speed_received = line.ReadChecked(value);
  if (!traffic.speed_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.speed = 0;
  } else
    traffic.speed = value;

  traffic.climb_rate_received = line.ReadChecked(value);
  if (!traffic.climb_rate_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.climb_rate = 0;
  } else
    traffic.climb_rate = value;

  traffic.stealth = stealth;

  unsigned type = line.Read(0);
  if (type > 15 || type == 14)
    traffic.type = FlarmTraffic::AircraftType::UNKNOWN;
  else
    traffic.type = (FlarmTraffic::AircraftType)type;

  FlarmTraffic *flarm_slot = flarm.FindTraffic(traffic.id);
  if (flarm_slot == nullptr) {
    flarm_slot = flarm.AllocateTraffic();
    if (flarm_slot == nullptr)
      // no more slots available
      return;

    flarm_slot->Clear();
    flarm_slot->id = traffic.id;

    flarm.new_traffic.Update(clock);
  }

  // set time of fix to current time
  flarm_slot->valid.Update(clock);

  flarm_slot->Update(traffic);
}
