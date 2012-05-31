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

#include "StaticParser.hpp"
#include "NMEA/InputLine.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"

void
ParsePFLAU(NMEAInputLine &line, FlarmStatus &flarm, fixed clock)
{
  flarm.available.Update(clock);

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  flarm.rx = line.read(0);
  flarm.tx = line.read(false);
  flarm.gps = (FlarmStatus::GPSStatus)
    line.read((int)FlarmStatus::GPSStatus::NONE);

  line.skip();
  flarm.alarm_level = (FlarmTraffic::AlarmType)
    line.read((int)FlarmTraffic::AlarmType::NONE);
}

void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, fixed clock)
{
  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  FlarmTraffic traffic;
  traffic.alarm_level = (FlarmTraffic::AlarmType)
    line.read((int)FlarmTraffic::AlarmType::NONE);

  fixed value;
  bool stealth = false;

  if (!line.read_checked(value))
    // Relative North is required !
    return;
  traffic.relative_north = value;

  if (!line.read_checked(value))
    // Relative East is required !
    return;
  traffic.relative_east = value;

  if (!line.read_checked(value))
    // Relative Altitude is required !
    return;
  traffic.relative_altitude = value;

  line.skip(); /* id type */

  // 5 id, 6 digit hex
  char id_string[16];
  line.read(id_string, 16);
  traffic.id = FlarmId::Parse(id_string, NULL);

  traffic.track_received = line.read_checked(value);
  if (!traffic.track_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.track = Angle::Zero();
  } else
    traffic.track = Angle::Degrees(value);

  traffic.turn_rate_received = line.read_checked(value);
  if (!traffic.turn_rate_received) {
    // Field is empty in stealth mode
    traffic.turn_rate = fixed_zero;
  } else
    traffic.turn_rate = value;

  traffic.speed_received = line.read_checked(value);
  if (!traffic.speed_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.speed = fixed_zero;
  } else
    traffic.speed = value;

  traffic.climb_rate_received = line.read_checked(value);
  if (!traffic.climb_rate_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.climb_rate = fixed_zero;
  } else
    traffic.climb_rate = value;

  traffic.stealth = stealth;

  unsigned type = line.read(0);
  if (type > 15 || type == 14)
    traffic.type = FlarmTraffic::AircraftType::UNKNOWN;
  else
    traffic.type = (FlarmTraffic::AircraftType)type;

  FlarmTraffic *flarm_slot = flarm.FindTraffic(traffic.id);
  if (flarm_slot == NULL) {
    flarm_slot = flarm.AllocateTraffic();
    if (flarm_slot == NULL)
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
