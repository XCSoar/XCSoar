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

#include "Device/Parser.hpp"
#include "Util/CharUtil.hpp"
#include "Geo/Geoid.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/System.hpp"
#include "Driver/FLARM/StaticParser.hpp"

NMEAParser::NMEAParser()
{
  Reset();
}

void
NMEAParser::Reset()
{
  real = true;
  use_geoid = true;
  last_time = 0;
}

bool
NMEAParser::ParseLine(const char *string, NMEAInfo &info)
{
  assert(info.clock > 0);

  if (string[0] != '$')
    return false;

  if (!NMEAChecksum(string))
    return false;

  NMEAInputLine line(string);

  char type[16];
  line.Read(type, 16);

  if (IsAlphaASCII(type[1]) && IsAlphaASCII(type[2])) {
    if (StringIsEqual(type + 3, "GSA"))
      return GSA(line, info);

    if (StringIsEqual(type + 3, "GLL"))
      return GLL(line, info);

    if (StringIsEqual(type + 3, "RMC"))
      return RMC(line, info);

    if (StringIsEqual(type + 3, "GGA"))
      return GGA(line, info);

    if (StringIsEqual(type + 3, "HDM"))
      return HDM(line, info);

    if (StringIsEqual(type + 3, "MWV"))
      return MWV(line, info);
  }

  // if (proprietary sentence) ...
  if (type[1] == 'P') {
    // Airspeed and vario sentence
    if (StringIsEqual(type + 1, "PTAS1"))
      return PTAS1(line, info);

    // FLARM sentences
    if (StringIsEqual(type + 1, "PFLAE")) {
      ParsePFLAE(line, info.flarm.error, info.clock);
      return true;
    }

    if (StringIsEqual(type + 1, "PFLAV")) {
      ParsePFLAV(line, info.flarm.version, info.clock);
      return true;
    }

    if (StringIsEqual(type + 1, "PFLAA")) {
      ParsePFLAA(line, info.flarm.traffic, info.clock);
      return true;
    }

    if (StringIsEqual(type + 1, "PFLAU")) {
      ParsePFLAU(line, info.flarm.status, info.clock);
      return true;
    }

    // Garmin altitude sentence
    if (StringIsEqual(type + 1, "PGRMZ"))
      return RMZ(line, info);

    return false;
  }

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

/**
 * Parses an angle in the form "DDDMM.SSS".  Minutes are 0..59, and
 * seconds are 0..999.
 */
static bool
ReadGeoAngle(NMEAInputLine &line, Angle &a)
{
  char buffer[32], *endptr;
  line.Read(buffer, sizeof(buffer));

  char *dot = strchr(buffer, '.');
  if (dot < buffer + 3)
    return false;

  double x = strtod(dot - 2, &endptr);
  if (x < 0 || x >= 60 || *endptr != 0)
    return false;

  dot[-2] = 0;
  long y = strtol(buffer, &endptr, 10);
  if (y < 0 || y > 180 || endptr == buffer || *endptr != 0)
    return false;

  a = Angle::Degrees(y + x / 60.);
  return true;
}

static bool
ReadDoubleAndChar(NMEAInputLine &line, double &d, char &ch)
{
  bool success = line.ReadChecked(d);
  ch = line.ReadFirstChar();
  return success;
}

static bool
ReadLatitude(NMEAInputLine &line, Angle &value_r)
{
  Angle value;
  if (!ReadGeoAngle(line, value))
    return false;

  char ch = line.ReadOneChar();
  if (ch == 'S')
    value.Flip();
  else if (ch != 'N')
    return false;

  value_r = value;
  return true;
}

static bool
ReadLongitude(NMEAInputLine &line, Angle &value_r)
{
  Angle value;
  if (!ReadGeoAngle(line, value))
    return false;

  char ch = line.ReadOneChar();
  if (ch == 'W')
    value.Flip();
  else if (ch != 'E')
    return false;

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
ReadAltitude(NMEAInputLine &line, double &value_r)
{
  double value;
  char format;
  if (!ReadDoubleAndChar(line, value, format))
    return false;

  if (format == 'f' || format == 'F')
    value = Units::ToSysUnit(value, Unit::FEET);

  value_r = value;
  return true;
}

bool
NMEAParser::TimeHasAdvanced(double this_time, NMEAInfo &info)
{
  return TimeHasAdvanced(this_time, last_time, info);
}

gcc_const
static bool
IsMidnightWraparound(double this_time, double last_time)
{
  constexpr unsigned SECONDS_PER_HOUR = 60 * 60;
  constexpr unsigned SECONDS_PER_DAY = 24 * SECONDS_PER_HOUR;

  return this_time < SECONDS_PER_HOUR &&
    last_time >= SECONDS_PER_DAY - SECONDS_PER_HOUR;
}

gcc_const
static bool
TimeHasAdvanced(double this_time, double last_time)
{
  return this_time >= last_time || IsMidnightWraparound(this_time, last_time);
}

bool
NMEAParser::TimeHasAdvanced(double this_time, double &last_time, NMEAInfo &info)
{
  if (!::TimeHasAdvanced(this_time, last_time)) {
    last_time = this_time;
    return false;
  } else {
    info.time = this_time;
    info.time_available.Update(info.clock);
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

  line.Skip();

  if (line.Read(0) == 1)
    info.location_available.Clear();

  // satellites are in items 4-15 of GSA string (4-15 is 1-indexed)
  for (unsigned i = 0; i < GPSState::MAXSATELLITES; i++)
    info.gps.satellite_ids[i] = line.Read(0);

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

  double this_time;
  if (!ReadTime(line, info.date_time_utc, this_time))
    return true;

  bool gps_valid = !NAVWarn(line.ReadFirstChar());

  if (!TimeHasAdvanced(this_time, info))
    return true;

  if (!gps_valid)
    info.location_available.Clear();
  else if (valid_location)
    info.location_available.Update(info.clock);

  if (valid_location)
    info.location = location;

  info.gps.real = real;
#if defined(ANDROID) || defined(__APPLE__)
  info.gps.nonexpiring_internal_gps = false;
#endif

  return true;
}

bool
NMEAParser::ReadDate(NMEAInputLine &line, BrokenDate &date)
{
  char buffer[9];
  line.Read(buffer, 9);

  if (strlen(buffer) != 6)
    return false;

  BrokenDate new_value;
  new_value.year = atoi(buffer + 4) + 2000;
  buffer[4] = '\0';
  new_value.month = atoi(buffer + 2);
  buffer[2] = '\0';
  new_value.day = atoi(buffer);
  new_value.day_of_week = -1;

  if (!new_value.IsPlausible())
    return false;

  date = new_value;
  return true;
}

bool
NMEAParser::ReadTime(NMEAInputLine &line, BrokenTime &broken_time,
                     double &time_of_day_s)
{
  double value;
  if (!line.ReadChecked(value) || value < 0)
    return false;

  // Calculate Hour
  unsigned hour = unsigned(value / 10000);
  if (hour >= 24)
    return false;

  // Calculate Minute
  unsigned minute = unsigned(value / 100) - hour * 100;
  if (minute >= 60)
    return false;

  // Calculate Second
  double second = value - (hour * 10000 + minute * 100);
  if (second >= 60)
    return false;

  broken_time = BrokenTime(hour, minute, (unsigned)second);
  time_of_day_s = (hour * 3600 + minute * 60) + second;
  return true;
}

/**
 * Parses unsigned floating-point deviation angle value in degrees.
 * and applies deviation sign from following E/W char
 */
static bool
ReadVariation(NMEAInputLine &line, Angle &value_r)
{
  double value;
  if (!line.ReadChecked(value))
    return false;
  char ch = line.ReadOneChar();
  if (ch == 'W')
    value = -value;
  else if (ch != 'E')
    return false;

  value_r = Angle::Degrees(value);
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

  double this_time;
  if (!ReadTime(line, info.date_time_utc, this_time))
    return true;

  bool gps_valid = !NAVWarn(line.ReadFirstChar());

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  double speed;
  bool ground_speed_available = line.ReadChecked(speed);

  Angle track;
  bool track_available = ReadBearing(line, track);

  // JMW get date info first so TimeModify is accurate
  ReadDate(line, info.date_time_utc);

  Angle variation;
  bool variation_available = ReadVariation(line, variation);

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
    info.track = track;
    info.track_available.Update(info.clock);
  }

  if (!variation_available)
    info.variation_available.Clear();
  else if (variation_available) {
    info.variation = variation;
    info.variation_available.Update(info.clock);
  }

  info.gps.real = real;
#if defined(ANDROID) || defined(__APPLE__)
  info.gps.nonexpiring_internal_gps = false;
#endif

  return true;
}

/**
 * Parse HDM NMEA sentence.
 */
bool
NMEAParser::HDM(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $HCHDM,238.5,M*hh/CR/LF
   *
   * Field Number:
   *  1) Magnetic Heading to one decimal place
   *  2) M (Magnetic)
   *  3) Checksum
   */
  Angle heading;
  bool heading_available = ReadBearing(line, heading);

  if (!heading_available)
    info.heading_available.Clear();
  else if (heading_available) {
    info.heading = heading;
    info.heading_available.Update(info.clock);
  }

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

  double this_time;
  if (!ReadTime(line, info.date_time_utc, this_time))
    return true;

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  unsigned fix_quality;
  if (line.ReadChecked(fix_quality)) {
    gps.fix_quality = (FixQuality)fix_quality;
    gps.fix_quality_available.Update(info.clock);
  }

  unsigned satellites_used;
  if (line.ReadChecked(satellites_used)) {
    info.gps.satellites_used = satellites_used;
    info.gps.satellites_used_available.Update(info.clock);
  }

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
#if defined(ANDROID) || defined(__APPLE__)
  info.gps.nonexpiring_internal_gps = false;
#endif

  gps.hdop = line.Read(0.);

  bool altitude_available = ReadAltitude(line, info.gps_altitude);
  if (altitude_available)
    info.gps_altitude_available.Update(info.clock);
  else
    info.gps_altitude_available.Clear();

  double geoid_separation;
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

  double value;
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
  double vario;
  if (line.ReadChecked(vario)) {
    // Properly convert to m/s
    vario = Units::ToSysUnit((vario - 200) / 10., Unit::KNOTS);
    info.ProvideTotalEnergyVario(vario);
  }

  // Skip average vario data
  line.Skip();

  // Parse barometric altitude
  double baro_altitude;
  if (line.ReadChecked(baro_altitude)) {
    // Properly convert to meter
    baro_altitude = Units::ToSysUnit(baro_altitude - 2000, Unit::FEET);
    info.ProvidePressureAltitude(baro_altitude);
  }

  // Parse true airspeed
  double vtas;
  if (line.ReadChecked(vtas))
    info.ProvideTrueAirspeed(Units::ToSysUnit(vtas, Unit::KNOTS));

  return true;
}

inline bool
NMEAParser::MWV(NMEAInputLine &line, NMEAInfo &info)
{
  /*
    * $--MWV,x.x,a,x.x,a,a,a,*hh
    *
    * Field Number:
    *  1) wind angle
    *  2) (R)elative or (T)rue
    *  3) wind speed
    *  4) K/M/N
    *  5) Status A=valid
    *  8) Checksum
    */

  Angle winddir;
  if (!ReadBearing(line, winddir))
    return false;

  char ch = line.ReadOneChar();

  double windspeed;
  if (!line.ReadChecked(windspeed))
    return false;

  ch = line.ReadOneChar();
  switch (ch) {
  case 'N':
    windspeed = Units::ToSysUnit(windspeed, Unit::KNOTS);
    break;

  case 'K':
    windspeed = Units::ToSysUnit(windspeed, Unit::KILOMETER_PER_HOUR);
    break;

  case 'M':
    windspeed = Units::ToSysUnit(windspeed, Unit::METER_PER_SECOND);
    break;

  default:
    return false;
  }

  SpeedVector wind(winddir, windspeed);
  info.ProvideExternalWind(wind);

  return true;
}
