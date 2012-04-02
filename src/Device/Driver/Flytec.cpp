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

#include "Device/Driver/Flytec.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Device/Port/Port.hpp"
#include "TimeoutClock.hpp"
#include "Util/Macros.hpp"
#include "IO/TextWriter.hpp"
#include "Operation/Operation.hpp"
#include "Replay/IGCParser.hpp"

#include <stdlib.h>
#include <math.h>

class FlytecDevice : public AbstractDevice {
  Port &port;

public:
  FlytecDevice(Port &_port):port(_port) {}

  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);

  bool ReadFlightList(RecordedFlightList &flight_list, OperationEnvironment &env);
  bool DownloadFlight(const RecordedFlightInfo &flight, const TCHAR *path,
                      OperationEnvironment &env);
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
    info.ProvideBothAirspeeds(Units::ToSysUnit(value, Unit::KILOMETER_PER_HOUR));

  // 10,11 = temperature, unit
  info.temperature_available =
    line.read_checked_compare(value, "C");
  if (info.temperature_available)
    info.temperature = CelsiusToKelvin(value);

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
      info.track = Angle::Degrees(track);
      info.track_available.Update(info.clock);
    }

    //  Speed over Ground (xxxxx dm/s), 5 Digits
    fixed ground_speed;
    if (line.read_checked(ground_speed)) {
      info.ground_speed = ground_speed / 10;
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
  if (line.read_checked(satellites_used)) {
    info.gps.satellites_used = satellites_used;
    info.gps.satellites_used_available.Update(info.clock);
  }

  //  Raw pressure (xxxxxx Pa),  6 Digits
  fixed pressure;
  if (line.read_checked(pressure))
    info.ProvideStaticPressure(AtmosphericPressure::Pascal(pressure));

  //  Baro Altitude (xxxxx meter),          5 Digits (-xxxx to xxxxx) (Based on 1013.25hPa)
  fixed baro_altitude;
  if (line.read_checked(baro_altitude))
    info.ProvidePressureAltitude(baro_altitude);

  //  Variometer (xxxx cm/s),   4 or 5 Digits (-9999 to 9999)
  fixed vario;
  if (line.read_checked(vario))
    info.ProvideTotalEnergyVario(vario / 100);

  //  True airspeed (xxxxx dm/s), 5 Digits
  fixed tas;
  if (line.read_checked(tas))
    info.ProvideTrueAirspeed(tas / 10);

  //  Airspeed source P or V,   1 Digit P= pitot, V = Vane wheel
  line.skip();

  //  Temp. PCB (xxx �C),   3 Digits
  fixed pcb_temperature;
  bool pcb_temperature_available = line.read_checked(pcb_temperature);

  //  Temp. Balloon Envelope (xxx �C),      3 Digits
  fixed balloon_temperature;
  bool balloon_temperature_available = line.read_checked(balloon_temperature);

  if (balloon_temperature_available) {
    info.temperature = CelsiusToKelvin(balloon_temperature);
    info.temperature_available = true;
  } else if (pcb_temperature_available) {
    info.temperature = CelsiusToKelvin(pcb_temperature);
    info.temperature_available = true;
  }

  //  Battery Capacity Bank 1 (0 to 100%)   3 Digits
  fixed battery_level_1;
  bool battery_level_1_available = line.read_checked(battery_level_1);

  //  Battery Capacity Bank 2 (0 to 100%)   3 Digits
  fixed battery_level_2;
  bool battery_level_2_available = line.read_checked(battery_level_2);

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
  line.read(type, 16);

  if (StringIsEqual(type, "$BRSF"))
    return FlytecParseBRSF(line, info);
  else if (StringIsEqual(type, "$VMVABD"))
    return FlytecParseVMVABD(line, info);
  else if (StringIsEqual(type, "$FLYSEN"))
    return FlytecParseFLYSEN(line, info);
  else
    return false;
}

static bool
ExpectXOff(Port &port, OperationEnvironment &env, unsigned timeout_ms)
{
  return port.WaitForChar(0x13, env, timeout_ms) == Port::WaitResult::READY;
}

static bool
ReceiveLine(Port &port, char *buffer, size_t length, unsigned timeout_ms)
{
  TimeoutClock timeout(timeout_ms);

  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.HasExpired())
      return false;

    // Read single character from port
    int c = port.GetChar();

    // On failure try again until timed out
    if (c == -1)
      continue;

    // Break on XOn
    if (c == 0x11) {
      *p = '\0';
      break;
    }

    // Write received character to buffer
    *p = c;
    p++;

    // Break on line break
    if (c == '\n') {
      *p = '\0';
      break;
    }
  }

  return true;
}

static bool
ParseDate(const char *str, BrokenDate &date)
{
  char *endptr;

  // Parse day
  date.day = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != '.')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse month
  date.month = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != '.')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse year
  date.year = strtoul(str, &endptr, 10) + 2000;

  // Check if parsed correctly and following character is a separator
  return str != endptr;
}

static bool
ParseTime(const char *str, BrokenTime &time)
{
  char *endptr;

  // Parse year
  time.hour = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != ':')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse month
  time.minute = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  if (str == endptr || *endptr != ':')
    return false;

  // Set str pointer to first character after the separator
  str = endptr + 1;

  // Parse day
  time.second = strtoul(str, &endptr, 10);

  // Check if parsed correctly and following character is a separator
  return str != endptr;
}

static BrokenTime
operator+(BrokenTime &a, BrokenTime &b)
{
  BrokenTime c;

  c.hour = a.hour + b.hour;
  c.minute = a.minute + b.minute;
  c.second = a.second + b.second;

  while (c.second >= 60) {
    c.second -= 60;
    c.minute++;
  }

  while (c.minute >= 60) {
    c.minute -= 60;
    c.hour++;
  }

  while (c.hour >= 23)
    c.hour -= 24;

  return c;
}

bool
FlytecDevice::ReadFlightList(RecordedFlightList &flight_list,
                             OperationEnvironment &env)
{
  port.StopRxThread();

  char buffer[256];
  strcpy(buffer, "$PBRTL,");
  AppendNMEAChecksum(buffer);
  strcat(buffer, "\r\n");

  port.Write(buffer);
  if (!ExpectXOff(port, env, 1000))
    return false;

  unsigned tracks = 0;
  while (true) {
    // Check if the user cancelled the operation
    if (env.IsCancelled())
      return false;

    // Receive the next line
    if (!ReceiveLine(port, buffer, ARRAY_SIZE(buffer), 1000))
      return false;

    // XON was received, last record was read already
    if (StringIsEmpty(buffer))
      break;

    // $PBRTL    Identifier
    // AA        total number of stored tracks
    // BB        actual number of track (0 indicates the most actual track)
    // DD.MM.YY  date of recorded track (UTC)(e.g. 24.03.04)
    // hh:mm:ss  starttime (UTC)(e.g. 08:23:15)
    // HH:MM:SS  duration (e.g. 03:23:15)
    // *ZZ       Checksum as defined by NMEA

    RecordedFlightInfo flight;
    NMEAInputLine line(buffer);

    // Skip $PBRTL
    line.skip();

    if (tracks == 0) {
      // If number of tracks not read yet
      // .. read and save it
      if (!line.read_checked(tracks))
        continue;

      env.SetProgressRange(tracks);
    } else
      line.skip();

    if (!line.read_checked(flight.internal.flytec))
      continue;

    if (tracks != 0 && flight.internal.flytec < tracks)
      env.SetProgressPosition(flight.internal.flytec);

    char field_buffer[16];
    line.read(field_buffer, ARRAY_SIZE(field_buffer));
    if (!ParseDate(field_buffer, flight.date))
      continue;

    line.read(field_buffer, ARRAY_SIZE(field_buffer));
    if (!ParseTime(field_buffer, flight.start_time))
      continue;

    BrokenTime duration;
    line.read(field_buffer, ARRAY_SIZE(field_buffer));
    if (!ParseTime(field_buffer, duration))
      continue;

    flight.end_time = flight.start_time + duration;
    flight_list.append(flight);
  }

  return true;
}

bool
FlytecDevice::DownloadFlight(const RecordedFlightInfo &flight,
                             const TCHAR *path, OperationEnvironment &env)
{
  port.StopRxThread();

  PeriodClock status_clock;
  status_clock.Update();

  // Request flight record
  char buffer[256];
  sprintf(buffer, "$PBRTR,%02d", flight.internal.flytec);
  AppendNMEAChecksum(buffer);
  strcat(buffer, "\r\n");

  port.Write(buffer);
  if (!ExpectXOff(port, env, 1000))
    return false;

  // Open file writer
  FileHandle writer(path, _T("wb"));
  if (!writer.IsOpen())
    return false;

  unsigned start_sec = flight.start_time.GetSecondOfDay();
  unsigned end_sec = flight.end_time.GetSecondOfDay();
  if (end_sec < start_sec)
    end_sec += 24 * 60 * 60;

  unsigned range = end_sec - start_sec;
  env.SetProgressRange(range);

  while (true) {
    // Check if the user cancelled the operation
    if (env.IsCancelled())
      return false;

    // Receive the next line
    if (!ReceiveLine(port, buffer, ARRAY_SIZE(buffer), 1000))
      return false;

    // XON was received
    if (StringIsEmpty(buffer))
      break;

    if (status_clock.CheckUpdate(250)) {
      // Parse the fix time
      BrokenTime time;
      if (IGCParseFixTime(buffer, time)) {

        unsigned time_sec = time.GetSecondOfDay();
        if (time_sec < start_sec)
          time_sec += 24 * 60 * 60;

        if (time_sec > end_sec + 5 * 60)
          time_sec = start_sec;

        unsigned position = time_sec - start_sec;
        if (position > range)
          position = range;

        env.SetProgressPosition(position);
      }
    }

    // Write line to the file
    writer.Write(buffer);
  }

  return true;
}

static Device *
FlytecCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new FlytecDevice(com_port);
}

const struct DeviceRegister flytec_device_driver = {
  _T("Flytec"),
  _T("Flytec 5030 / Brauniger"),
  0 /* DeviceRegister::LOGGER deactivated until current firmware supports this */,
  FlytecCreateOnPort,
};
