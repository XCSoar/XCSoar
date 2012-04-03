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

#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Geo/Geoid.hpp"
#include "Math/Earth.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "Util/StringUtil.hpp"
#include "Compatibility/string.h" /* for _ttoi() */
#include "Units/System.hpp"
#include "OS/Clock.hpp"

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>

using std::min;
using std::max;

int NMEAParser::start_day = -1;

NMEAParser::NMEAParser(bool _ignore_checksum)
  :ignore_checksum(_ignore_checksum)
{
  Reset();
}

void
NMEAParser::Reset()
{
  real = true;
  use_geoid = true;
  last_time = fixed_zero;
}

bool
NMEAParser::ParseLine(const char *string, NMEAInfo &info)
{
  assert(positive(info.clock));

  if (string[0] != '$')
    return false;

  if (!ignore_checksum && !NMEAChecksum(string))
    return false;

  NMEAInputLine line(string);

  char type[16];
  line.read(type, 16);

  // if (proprietary sentence) ...
  if (type[1] == 'P') {
    // Airspeed and vario sentence
    if (StringIsEqual(type + 1, "PTAS1"))
      return PTAS1(line, info);

    // FLARM sentences
    if (StringIsEqual(type + 1, "PFLAA"))
      return PFLAA(line, info);

    if (StringIsEqual(type + 1, "PFLAU"))
      return PFLAU(line, info.flarm, info.clock);

    // Garmin altitude sentence
    if (StringIsEqual(type + 1, "PGRMZ"))
      return RMZ(line, info);

    return false;
  }

  if (StringIsEqual(type + 3, "GSA"))
    return GSA(line, info);

  if (StringIsEqual(type + 3, "GLL"))
    return GLL(line, info);

  if (StringIsEqual(type + 3, "RMC"))
    return RMC(line, info);

  if (StringIsEqual(type + 3, "GGA"))
    return GGA(line, info);

  return false;
}

/**
 * Parses whether the given character (GPS status) should create a navigational warning
 * @param c GPS status
 * @return True if GPS fix not found or invalid
 */
static bool
NAVWarn(char c)
{
  return c != 'A';
}

/**
 * Parses an angle in the form "DDDMM.SSS".  Minutes are 0..59, and
 * seconds are 0..999.
 */
static bool
ReadPositiveAngle(NMEAInputLine &line, Angle &a)
{
  char buffer[32], *endptr;
  line.read(buffer, sizeof(buffer));

  char *dot = strchr(buffer, '.');
  if (dot < buffer + 3)
    return false;

  double x = strtod(dot - 2, &endptr);
  if (x < 0 || x >= 60 || *endptr != 0)
    return false;

  dot[-2] = 0;
  long y = strtol(buffer, &endptr, 10);
  if (y < 0 || endptr == buffer || *endptr != 0)
    return false;

  a = Angle::Degrees(fixed(y) + fixed(x) / 60);
  return true;
}

static bool
ReadFixedAndChar(NMEAInputLine &line, fixed &d, char &ch)
{
  bool success = line.read_checked(d);
  ch = line.read_first_char();
  return success;
}

static bool
ReadLatitude(NMEAInputLine &line, Angle &value_r)
{
  Angle value;
  if (!ReadPositiveAngle(line, value))
    return false;

  if (line.read_first_char() == 'S')
    value.Flip();

  value_r = value;
  return true;
}

static bool
ReadLongitude(NMEAInputLine &line, Angle &value_r)
{
  Angle value;
  if (!ReadPositiveAngle(line, value))
    return false;

  if (line.read_first_char() == 'W')
    value.Flip();

  value_r = value;
  return true;
}

bool
NMEAParser::ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r)
{
  GeoPoint value;

  bool latitude_valid = ReadLatitude(line, value.latitude);
  bool longitude_valid = ReadLongitude(line, value.longitude);
  if (latitude_valid && longitude_valid) {
    value_r = value;
    return true;
  } else
    return false;
}

/**
 * Reads an altitude value, and the unit from a second volumn.
 */
static bool
ReadAltitude(NMEAInputLine &line, fixed &value_r)
{
  fixed value;
  char format;
  if (!ReadFixedAndChar(line, value, format))
    return false;

  if (format == 'f' || format == 'F')
    value = Units::ToSysUnit(value, Unit::FEET);

  value_r = value;
  return true;
}

fixed
NMEAParser::TimeModify(fixed fix_time, BrokenDateTime &date_time,
                       bool date_available)
{
  assert(!date_available || BrokenDate(date_time).Plausible());

  fixed hours, mins, secs;

  // Calculate Hour
  hours = fix_time / 10000;
  date_time.hour = (int)hours;

  // Calculate Minute
  mins = fix_time / 100;
  mins = mins - fixed(date_time.hour) * 100;
  date_time.minute = (int)mins;

  // Calculate Second
  secs = fix_time - fixed(date_time.hour * 10000 + date_time.minute * 100);
  date_time.second = (int)secs;

  // FixTime is now seconds-based instead of mixed format
  fix_time = secs + fixed(date_time.minute * 60 + date_time.hour * 3600);

  // If (StartDay not yet set and available) set StartDate;
  if (start_day == -1 && date_available)
    start_day = date_time.day;

  if (start_day != -1) {
    if (date_time.day < start_day)
      // detect change of month (e.g. day=1, startday=31)
      start_day = date_time.day - 1;

    int day_difference = date_time.day - start_day;
    if (day_difference > 0)
      // Add seconds to fix time so time doesn't wrap around when
      // going past midnight in UTC
      fix_time += fixed(day_difference * 86400);
  }

  return fix_time;
}

fixed
NMEAParser::TimeAdvanceTolerance(fixed time) const
{
  /* tolerance is two seconds: fast-forward if the new time stamp is
     less than two seconds behind the previous one */
  return time < last_time && time > last_time - fixed_two
    ? last_time
    : time;
}

bool
NMEAParser::TimeHasAdvanced(fixed this_time, NMEAInfo &info)
{
  if (this_time < last_time) {
    last_time = this_time;
    start_day = -1; // reset search for the first day
    return false;
  } else {
    info.time = this_time;
    info.time_available.Update(fixed(MonotonicClockMS()) / 1000);
    last_time = this_time;
    return true;
  }
}

bool
NMEAParser::GSA(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $--GSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
   *
   * Field Number:
   *  1) Selection mode
   *         M=Manual, forced to operate in 2D or 3D
   *         A=Automatic, 3D/2D
   *  2) Mode (1 = no fix, 2 = 2D fix, 3 = 3D fix)
   *  3) ID of 1st satellite used for fix
   *  4) ID of 2nd satellite used for fix
   *  ...
   *  14) ID of 12th satellite used for fix
   *  15) PDOP
   *  16) HDOP
   *  17) VDOP
   *  18) checksum
   */

  line.skip();

  if (line.read(0) == 1)
    info.location_available.Clear();

  // satellites are in items 4-15 of GSA string (4-15 is 1-indexed)
  for (unsigned i = 0; i < GPSState::MAXSATELLITES; i++)
    info.gps.satellite_ids[i] = line.read(0);

  info.gps.satellite_ids_available.Update(info.clock);

  return true;
}

bool
NMEAParser::GLL(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,a,m,*hh
   *
   * Field Number:
   *  1) Latitude
   *  2) N or S (North or South)
   *  3) Longitude
   *  4) E or W (East or West)
   *  5) Universal Time Coordinated (UTC)
   *  6) Status A - Data Valid, V - Data Invalid
   *  7) FAA mode indicator (NMEA 2.3 and later)
   *  8) Checksum
   */

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  fixed this_time = TimeModify(line.read(fixed_zero), info.date_time_utc,
                               info.date_available);
  this_time = TimeAdvanceTolerance(this_time);

  bool gps_valid = !NAVWarn(line.read_first_char());

  if (!TimeHasAdvanced(this_time, info))
    return true;

  if (!gps_valid)
    info.location_available.Clear();
  else if (valid_location)
    info.location_available.Update(info.clock);

  if (valid_location)
    info.location = location;

  info.gps.real = real;
#ifdef ANDROID
  info.gps.android_internal_gps = false;
#endif

  return true;
}

static bool
ReadDate(NMEAInputLine &line, BrokenDate &date)
{
  char buffer[9];
  line.read(buffer, 9);

  if (strlen(buffer) != 6)
    return false;

  BrokenDate new_value;
  new_value.year = atoi(buffer + 4) + 2000;
  buffer[4] = '\0';
  new_value.month = atoi(buffer + 2);
  buffer[2] = '\0';
  new_value.day = atoi(buffer);

  if (!new_value.Plausible())
    return false;

  date = new_value;
  return true;
}

bool
NMEAParser::RMC(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a,m,*hh
   *
   * Field Number:
   *  1) UTC Time
   *  2) Status, V=Navigation receiver warning A=Valid
   *  3) Latitude
   *  4) N or S
   *  5) Longitude
   *  6) E or W
   *  7) Speed over ground, knots
   *  8) Track made good, degrees true
   *  9) Date, ddmmyy
   * 10) Magnetic Variation, degrees
   * 11) E or W
   * 12) FAA mode indicator (NMEA 2.3 and later)
   * 13) Checksum
   */

  fixed this_time = line.read(fixed_zero);

  bool gps_valid = !NAVWarn(line.read_first_char());

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  fixed speed;
  bool ground_speed_available = line.read_checked(speed);

  fixed track;
  bool track_available = line.read_checked(track);

  // JMW get date info first so TimeModify is accurate
  if (ReadDate(line, info.date_time_utc))
    info.date_available = true;

  this_time = TimeModify(this_time, info.date_time_utc, info.date_available);
  this_time = TimeAdvanceTolerance(this_time);

  if (!TimeHasAdvanced(this_time, info))
    return true;

  if (!gps_valid)
    info.location_available.Clear();
  else if (valid_location)
    info.location_available.Update(info.clock);

  if (valid_location)
    info.location = location;

  if (ground_speed_available) {
    info.ground_speed = Units::ToSysUnit(speed, Unit::KNOTS);
    info.ground_speed_available.Update(info.clock);
  }

  if (track_available && info.MovementDetected()) {
    // JMW don't update bearing unless we're moving
    info.track = Angle::Degrees(track).AsBearing();
    info.track_available.Update(info.clock);
  }

  info.gps.real = real;
#ifdef ANDROID
  info.gps.android_internal_gps = false;
#endif

  return true;
}

bool
NMEAParser::GGA(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
   *
   * Field Number:
   *  1) Universal Time Coordinated (UTC)
   *  2) Latitude
   *  3) N or S (North or South)
   *  4) Longitude
   *  5) E or W (East or West)
   *  6) GPS Quality Indicator,
   *     0 - fix not available,
   *     1 - GPS fix,
   *     2 - Differential GPS fix
   *     (values above 2 are 2.3 features)
   *     3 = PPS fix
   *     4 = Real Time Kinematic
   *     5 = Float RTK
   *     6 = estimated (dead reckoning)
   *     7 = Manual input mode
   *     8 = Simulation mode
   *  7) Number of satellites in view, 00 - 12
   *  8) Horizontal Dilution of precision (meters)
   *  9) Antenna Altitude above/below mean-sea-level (geoid) (in meters)
   * 10) Units of antenna altitude, meters
   * 11) Geoidal separation, the difference between the WGS-84 earth
   *     ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level
   *     below ellipsoid
   * 12) Units of geoidal separation, meters
   * 13) Age of differential GPS data, time in seconds since last SC104
   *     type 1 or 9 update, null field when DGPS is not used
   * 14) Differential reference station ID, 0000-1023
   * 15) Checksum
   */
  GPSState &gps = info.gps;

  fixed this_time = TimeModify(line.read(fixed_zero), info.date_time_utc,
                               info.date_available);
  this_time = TimeAdvanceTolerance(this_time);

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  if (line.read_checked(gps.fix_quality))
    gps.fix_quality_available.Update(info.clock);

  gps.satellites_used_available.Update(info.clock);
  gps.satellites_used = min(16, line.read(-1));

  if (!TimeHasAdvanced(this_time, info))
    return true;

  (void)valid_location;
  /* JMW: note ignore location updates from GGA -- definitive frame is GPRMC sentence
  if (!gpsValid)
    info.LocationAvailable.Clear();
  else if (valid_location)
    info.LocationAvailable.Update(info.clock);

  if (valid_location)
    info.Location = location;
  */

  info.gps.real = real;
#ifdef ANDROID
  info.gps.android_internal_gps = false;
#endif

  gps.hdop = line.read(fixed_zero);

  bool altitude_available = ReadAltitude(line, info.gps_altitude);
  if (altitude_available)
    info.gps_altitude_available.Update(info.clock);
  else
    info.gps_altitude_available.Clear();

  fixed geoid_separation;
  if (ReadAltitude(line, geoid_separation)) {
    // No real need to parse this value,
    // but we do assume that no correction is required in this case

    if (!altitude_available) {
      /* Some devices, such as the "LG Incite Cellphone" seem to be
         severely bugged, and report the GPS altitude in the Geoid
         column.  That sucks! */
      info.gps_altitude = geoid_separation;
      info.gps_altitude_available.Update(info.clock);
    }
  } else {
    // need to estimate Geoid Separation internally (optional)
    // FLARM uses MSL altitude
    //
    // Some others don't.
    //
    // If the separation doesn't appear in the sentence,
    // we can assume the GPS unit is giving ellipsoid height
    //
    if (use_geoid) {
      // JMW TODO really need to know the actual device..
      geoid_separation = EGM96::LookupSeparation(info.location);
      info.gps_altitude -= geoid_separation;
    }
  }

  return true;
}

bool
NMEAParser::RMZ(NMEAInputLine &line, NMEAInfo &info)
{
  //JMW?  RMZAltitude = info.pressure.PressureAltitudeToQNHAltitude(RMZAltitude);

  fixed value;
  if (ReadAltitude(line, value)) {
    // JMW no in-built baro sources, so use this generic one
    if (info.flarm.IsDetected()) {
      /* FLARM emulates the Garmin $PGRMZ sentence, but emits the
         altitude above 1013.25 hPa - since the don't have a "FLARM"
         device driver, we use the auto-detected "isFlarm" flag
         here */
      info.ProvideWeakPressureAltitude(value);

      /* when a FLARM gets detected too late, the previous call to
         this function may have filled the PGRMZ value into
         "barometric altitude"; that was a misapprehension, and the
         following line attempts to correct it as early as possible */
      info.ClearWeakBaroAltitude();
    } else {
      info.ProvideWeakBaroAltitude(value);
      info.ClearWeakPressureAltitude();
    }
  }

  return true;
}

bool
NMEAParser::NMEAChecksum(const char *string)
{
  return VerifyNMEAChecksum(string);
}

bool
NMEAParser::PTAS1(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $PTAS1,xxx,yyy,zzzzz,aaa*CS<CR><LF>
   *
   * xxx
   * CV or current vario. =vario*10+200 range 0-400(display +/-20.0 knots)
   *
   * yyy
   * AV or average vario. =vario*10+200 range 0-400(display +/-20.0 knots)
   *
   * zzzzz
   * Barometric altitude in feet +2000
   *
   * aaa
   * TAS knots 0-200
   */

  // Parse current vario data
  fixed vario;
  if (line.read_checked(vario)) {
    // Properly convert to m/s
    vario = Units::ToSysUnit((vario - fixed(200)) / 10, Unit::KNOTS);
    info.ProvideTotalEnergyVario(vario);
  }

  // Skip average vario data
  line.skip();

  // Parse barometric altitude
  fixed baro_altitude;
  if (line.read_checked(baro_altitude)) {
    // Properly convert to meter
    baro_altitude = Units::ToSysUnit(baro_altitude - fixed(2000), Unit::FEET);
    info.ProvidePressureAltitude(baro_altitude);
  }

  // Parse true airspeed
  fixed vtas;
  if (line.read_checked(vtas))
    info.ProvideTrueAirspeed(Units::ToSysUnit(vtas, Unit::KNOTS));

  return true;
}

bool
NMEAParser::PFLAU(NMEAInputLine &line, FlarmState &flarm, fixed time)
{
  flarm.available.Update(time);

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  flarm.rx = line.read(0);
  flarm.tx = line.read(false);
  flarm.gps = (FlarmState::GPSStatus)
    line.read((int)FlarmState::GPSStatus::NONE);

  line.skip();
  flarm.alarm_level = (FlarmTraffic::AlarmType)
    line.read((int)FlarmTraffic::AlarmType::NONE);

  return true;
}

bool
NMEAParser::PFLAA(NMEAInputLine &line, NMEAInfo &info)
{
  FlarmState &flarm = info.flarm;

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  FlarmTraffic traffic;
  traffic.alarm_level = (FlarmTraffic::AlarmType)
    line.read((int)FlarmTraffic::AlarmType::NONE);

  fixed value;
  bool stealth = false;

  if (!line.read_checked(value))
    // Relative North is required !
    return true;
  traffic.relative_north = value;

  if (!line.read_checked(value))
    // Relative East is required !
    return true;
  traffic.relative_east = value;

  if (!line.read_checked(value))
    // Relative Altitude is required !
    return true;
  traffic.relative_altitude = value;

  line.skip(); /* id type */

  // 5 id, 6 digit hex
  char id_string[16];
  line.read(id_string, 16);
  traffic.id.Parse(id_string, NULL);

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
      return true;

    flarm_slot->Clear();
    flarm_slot->id = traffic.id;

    flarm.new_traffic = true;
  }

  // set time of fix to current time
  flarm_slot->valid.Update(info.clock);

  flarm_slot->Update(traffic);

  return true;
}
