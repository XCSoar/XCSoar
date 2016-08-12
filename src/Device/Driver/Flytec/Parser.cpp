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

#include "Device.hpp"
#include "Device/Parser.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"

/**
 * Parse a "$BRSF" sentence.
 *
 * Example: "$BRSF,063,-013,-0035,1,193,00351,535,485*38"
 */
static bool
FlytecParseBRSF(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // 0 = indicated or true airspeed [km/h]
  if (line.ReadChecked(value))
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  // 1 = integrated vario [dm/s]
  // 2 = altitude A2 [m] (XXX what's this?)
  // 3 = waypoint
  // 4 = bearing to waypoint [degrees]
  // 5 = distance to waypoint [100m]
  // 6 = MacCready speed to fly [100m/h]
  // 7 = speed to fly, best glide [100m/h]

  return true;
}

/**
 * Parse a "$VMVABD" sentence.
 *
 * Example: "$VMVABD,0000.0,M,0547.0,M,-0.0,,,MS,0.0,KH,22.4,C*65"
 */
static bool
FlytecParseVMVABD(NMEAInputLine &line, NMEAInfo &info)
{
  double value;

  // 0,1 = GPS altitude, unit
  if (line.ReadCheckedCompare(info.gps_altitude, "M"))
    info.gps_altitude_available.Update(info.clock);

  // 2,3 = baro altitude, unit
  if (line.ReadCheckedCompare(value, "M"))
    info.ProvideBaroAltitudeTrue(value);

  // 4-7 = integrated vario, unit
  line.Skip(4);

  // 8,9 = indicated or true airspeed, unit
  if (line.ReadCheckedCompare(value, "KH"))
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  // 10,11 = temperature, unit
  info.temperature_available =
    line.ReadCheckedCompare(value, "C");
  if (info.temperature_available)
    info.temperature = Temperature::FromCelsius(value);

  return true;
}

/**
 * Parse a "$FLYSEN" sentence.
 *
 * @see http://www.flytec.ch/public/Special%20NMEA%20sentence.pdf
 */
bool
FlytecDevice::ParseFLYSEN(NMEAInputLine &line, NMEAInfo &info)
{
  // Detect firmware/sentence version
  //
  // V or A in field 9  -> 3.31-
  // V or A in field 10 -> 3.32+

  NMEAInputLine line_copy(line);

  line_copy.Skip(8);

  bool has_date_field = false;
  char validity = line_copy.ReadFirstChar();
  if (validity != 'A' && validity != 'V') {
    validity = line_copy.ReadFirstChar();
    if (validity != 'A' && validity != 'V')
      return false;

    has_date_field = true;
  }

  //  Date(ddmmyy),   6 Digits (only in firmware version 3.32+)
  if (has_date_field)
    NMEAParser::ReadDate(line, info.date_time_utc);

  //  Time(hhmmss),   6 Digits

  double time;
  if (NMEAParser::ReadTime(line, info.date_time_utc, time) &&
      !NMEAParser::TimeHasAdvanced(time, last_time, info))
    return true;

  if (validity == 'V') {
    // In case of V (void=not valid) GPS data should not be used.
    // GPS altitude, position and speed should be ignored.
    line.Skip(7);

  } else {
    //  Latitude(ddmm.mmm),   8 Digits incl. decimal
    //  N (or S),   1 Digit
    //  Longitude(dddmm.mmm),   9 Digits inc. decimal
    //  E (or W),   1 Digit
    GeoPoint location;
    if (NMEAParser::ReadGeoPoint(line, location)) {
      info.location = location;
      info.location_available.Update(info.clock);
    }

    //  Track (xxx Deg),   3 Digits
    double track;
    if (line.ReadChecked(track)) {
      info.track = Angle::Degrees(track);
      info.track_available.Update(info.clock);
    }

    //  Speed over Ground (xxxxx dm/s), 5 Digits
    double ground_speed;
    if (line.ReadChecked(ground_speed)) {
      info.ground_speed = ground_speed / 10;
      info.ground_speed_available.Update(info.clock);
    }

    //  GPS altitude (xxxxx meter),           5 Digits
    double gps_altitude;
    if (line.ReadChecked(gps_altitude)) {
      info.gps_altitude = gps_altitude;
      info.gps_altitude_available.Update(info.clock);
    }
  }

  //  Validity of 3 D fix A or V,           1 Digit
  line.Skip();

  //  Satellites in Use (0 to 12),          2 Digits
  unsigned satellites_used;
  if (line.ReadChecked(satellites_used)) {
    info.gps.satellites_used = satellites_used;
    info.gps.satellites_used_available.Update(info.clock);
  }

  //  Raw pressure (xxxxxx Pa),  6 Digits
  double pressure;
  if (line.ReadChecked(pressure))
    info.ProvideStaticPressure(AtmosphericPressure::Pascal(pressure));

  //  Baro Altitude (xxxxx meter),          5 Digits (-xxxx to xxxxx) (Based on 1013.25hPa)
  double baro_altitude;
  if (line.ReadChecked(baro_altitude))
    info.ProvidePressureAltitude(baro_altitude);

  //  Variometer (xxxx cm/s),   4 or 5 Digits (-9999 to 9999)
  double vario;
  if (line.ReadChecked(vario))
    info.ProvideTotalEnergyVario(vario / 100);

  //  True airspeed (xxxxx dm/s), 5 Digits
  double tas;
  if (line.ReadChecked(tas))
    info.ProvideTrueAirspeed(tas / 10);

  //  Airspeed source P or V,   1 Digit P= pitot, V = Vane wheel
  line.Skip();

  //  Temp. PCB (xxx °C),   3 Digits
  double pcb_temperature;
  bool pcb_temperature_available = line.ReadChecked(pcb_temperature);

  //  Temp. Balloon Envelope (xxx °C),      3 Digits
  double balloon_temperature;
  bool balloon_temperature_available = line.ReadChecked(balloon_temperature);

  if (balloon_temperature_available) {
    info.temperature = Temperature::FromCelsius(balloon_temperature);
    info.temperature_available = true;
  } else if (pcb_temperature_available) {
    info.temperature = Temperature::FromCelsius(pcb_temperature);
    info.temperature_available = true;
  }

  //  Battery Capacity Bank 1 (0 to 100%)   3 Digits
  double battery_level_1;
  bool battery_level_1_available = line.ReadChecked(battery_level_1);

  //  Battery Capacity Bank 2 (0 to 100%)   3 Digits
  double battery_level_2;
  bool battery_level_2_available = line.ReadChecked(battery_level_2);

  if (battery_level_1_available) {
    if (battery_level_2_available)
      info.battery_level = (battery_level_1 + battery_level_2) / 2;
    else
      info.battery_level = battery_level_1;

    info.battery_level_available.Update(info.clock);
  } else if (battery_level_2_available) {
    info.battery_level = battery_level_2;
    info.battery_level_available.Update(info.clock);
  }

  //  Dist. to WP (xxxxxx m),   6 Digits (Max 200000m)
  //  Bearing (xxx Deg),   3 Digits
  //  Speed to fly1 (MC0 xxxxx cm/s),       5 Digits
  //  Speed to fly2 (McC. xxxxx cm/s)       5 Digits
  //  Keypress Code (Experimental empty to 99)     2 Digits

  return true;
}

bool
FlytecDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);
  char type[16];
  line.Read(type, 16);

  if (StringIsEqual(type, "$BRSF"))
    return FlytecParseBRSF(line, info);
  else if (StringIsEqual(type, "$VMVABD"))
    return FlytecParseVMVABD(line, info);
  else if (StringIsEqual(type, "$FLYSEN"))
    return ParseFLYSEN(line, info);
  else
    return false;
}
