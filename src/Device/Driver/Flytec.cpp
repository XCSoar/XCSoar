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

#include "Device/Driver/Flytec.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Units.hpp"

#include <stdlib.h>
#include <math.h>

class FlytecDevice : public AbstractDevice {
private:
  Port *port;

public:
  FlytecDevice(Port *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);
};

/**
 * Parse a "$BRSF" sentence.
 *
 * Example: "$BRSF,063,-013,-0035,1,193,00351,535,485*38"
 */
static bool
FlytecParseBRSF(NMEAInputLine &line, NMEAInfo &info)
{
  fixed value;

  // 0 = indicated or true airspeed [km/h]
  if (line.read_checked(value))
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, unKiloMeterPerHour));

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
  fixed value;

  // 0,1 = GPS altitude, unit
  if (line.read_checked_compare(info.gps_altitude, "M"))
    info.gps_altitude_available.Update(info.clock);

  // 2,3 = baro altitude, unit
  if (line.read_checked_compare(value, "M"))
    info.ProvideBaroAltitudeTrue(value);

  // 4-7 = integrated vario, unit
  line.skip(4);

  // 8,9 = indicated or true airspeed, unit
  if (line.read_checked_compare(value, "KH"))
    // XXX is that TAS or IAS?  Documentation isn't clear.
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, unKiloMeterPerHour));

  // 10,11 = temperature, unit
  info.temperature_available =
    line.read_checked_compare(value, "C");
  if (info.temperature_available)
    info.temperature = Units::ToSysUnit(value, unGradCelcius);

  return true;
}

/**
 * Parse a "$FLYSEN" sentence.
 *
 * @see http://www.flytec.ch/public/Special%20NMEA%20sentence.pdf
 */
static bool
FlytecParseFLYSEN(NMEAInputLine &line, NMEAInfo &info)
{
  // Detect firmware/sentence version
  //
  // V or A in field 9  -> 3.31-
  // V or A in field 10 -> 3.32+

  NMEAInputLine line_copy(line.rest());

  line_copy.skip(8);

  bool has_date_field = false;
  char validity = line_copy.read_first_char();
  if (validity != 'A' && validity != 'V') {
    validity = line_copy.read_first_char();
    if (validity != 'A' && validity != 'V')
      return false;

    has_date_field = true;
  }

  //  Date(ddmmyy),   6 Digits (only in firmware version 3.32+)
  if (has_date_field)
    line.skip();

  //  Time(hhmmss),   6 Digits
  line.skip();

  if (validity == 'V') {
    // In case of V (void=not valid) GPS data should not be used.
    // GPS altitude, position and speed should be ignored.
    line.skip(7);

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
    fixed track;
    if (line.read_checked(track)) {
      info.track = Angle::degrees(track);
      info.track_available.Update(info.clock);
    }

    //  Speed over Ground (xxxxx cm/s)        5 Digits
    fixed ground_speed;
    if (line.read_checked(ground_speed)) {
      info.ground_speed = ground_speed / 100;
      info.ground_speed_available.Update(info.clock);
    }

    //  GPS altitude (xxxxx meter),           5 Digits
    fixed gps_altitude;
    if (line.read_checked(gps_altitude)) {
      info.gps_altitude = gps_altitude;
      info.gps_altitude_available.Update(info.clock);
    }
  }

  //  Validity of 3 D fix A or V,           1 Digit
  line.skip();

  //  Satellites in Use (0 to 12),          2 Digits
  unsigned satellites_used;
  if (line.read_checked(satellites_used) )
    info.gps.satellites_used = satellites_used;

  //  Raw pressure (xxxxxx Pa),  6 Digits
  fixed pressure;
  if (line.read_checked(pressure))
    info.ProvideStaticPressure(pressure);

  //  Baro Altitude (xxxxx meter),          5 Digits (-xxxx to xxxxx) (Based on 1013.25hPa)
  fixed baro_altitude;
  if (line.read_checked(baro_altitude))
    info.ProvidePressureAltitude(baro_altitude);

  //  Variometer (xxxx cm/s),   4 or 5 Digits (-9999 to 9999)
  fixed vario;
  if (line.read_checked(vario))
    info.ProvideTotalEnergyVario(vario / 100);

  //  True airspeed (xxxxx cm/s),           5 Digits (0 to 99999cm/s = 3600km/h)
  fixed tas;
  if (line.read_checked(tas))
    info.ProvideTrueAirspeed(tas / 100);

  //  Airspeed source P or V,   1 Digit P= pitot, V = Vane wheel
  //  Temp. PCB (xxx �C),   3 Digits
  //  Temp. Balloon Envelope (xxx �C),      3 Digits
  //  Battery Capacity Bank 1 (0 to 100%)   3 Digits
  //  Battery Capacity Bank 2 (0 to 100%)   3 Digits
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
  NMEAInputLine line(_line);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$BRSF") == 0)
    return FlytecParseBRSF(line, info);
  else if (strcmp(type, "$VMVABD") == 0)
    return FlytecParseVMVABD(line, info);
  else if (strcmp(type, "$FLYSEN") == 0)
    return FlytecParseFLYSEN(line, info);
  else
    return false;
}

static Device *
FlytecCreateOnPort(const DeviceConfig &config, Port *com_port)
{
  return new FlytecDevice(com_port);
}

const struct DeviceRegister flytec_device_driver = {
  _T("Flytec"),
  _T("Flytec 5030 / Brauniger"),
  0,
  FlytecCreateOnPort,
};
