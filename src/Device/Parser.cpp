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

#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Device/Geoid.h"
#include "Math/Earth.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "StringUtil.hpp"
#include "Compatibility/string.h" /* for _ttoi() */
#include "Units/Units.hpp"

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>

using std::min;
using std::max;

bool NMEAParser::ignore_checksum;

int NMEAParser::StartDay = -1;

/**
 * Constructor of the NMEAParser class
 * @return NMEAParser object
 */
NMEAParser::NMEAParser() {
  Reset();
}

/**
 * Resets the NMEAParser
 */
void
NMEAParser::Reset(void)
{
  real = true;
  gpsValid = false;
  isFlarm = false;
  GGAAvailable = false;
  LastTime = fixed_zero;
}

/**
 * Parses a provided NMEA String into a GPS_INFO struct
 * @param String NMEA string
 * @param GPS_INFO GPS_INFO output struct
 * @return Parsing success
 */
bool
NMEAParser::ParseNMEAString_Internal(const char *String, NMEA_INFO *GPS_INFO)
{
  if (String[0] != '$')
    return false;

  if (!NMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  char type[16];
  line.read(type, 16);

  // if (proprietary sentence) ...
  if (type[1] == 'P') {
    // Airspeed and vario sentence
    if (strcmp(type + 1, "PTAS1") == 0)
      return PTAS1(line, GPS_INFO);

    // FLARM sentences
    if (strcmp(type + 1, "PFLAA") == 0)
      return PFLAA(line, GPS_INFO);

    if (strcmp(type + 1, "PFLAU") == 0)
      return PFLAU(line, GPS_INFO->flarm, GPS_INFO->Time);

    // Garmin altitude sentence
    if (strcmp(type + 1, "PGRMZ") == 0)
      return RMZ(line, GPS_INFO);

    return false;
  }

  if (strcmp(type + 3, "GSA") == 0)
    return GSA(line, GPS_INFO);

  if (strcmp(type + 3, "GLL") == 0)
    return GLL(line, GPS_INFO);

  if (strcmp(type + 3, "RMB") == 0)
    return RMB(line, GPS_INFO);

  if (strcmp(type + 3, "RMC") == 0)
    return RMC(line, GPS_INFO);

  if (strcmp(type + 3, "GGA") == 0)
    return GGA(line, GPS_INFO);

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

  a = Angle::degrees(fixed(y) + fixed(x) / 60);
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
    value = -value;

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
    value = -value;

  value_r = value;
  return true;
}

static bool
ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r)
{
  GeoPoint value;

  bool latitude_valid = ReadLatitude(line, value.Latitude);
  bool longitude_valid = ReadLongitude(line, value.Longitude);
  if (latitude_valid && longitude_valid) {
    value_r = value;
    return true;
  } else
    return false;
}

bool
NMEAParser::ReadAltitude(NMEAInputLine &line, fixed &value_r)
{
  fixed value;
  char format;
  if (!ReadFixedAndChar(line, value, format))
    return false;

  if (format == 'f' || format == 'F')
    value = Units::ToSysUnit(value, unFeet);

  value_r = value;
  return true;
}

/**
 * Calculates a seconds-based FixTime and corrects it
 * in case over passing the UTC midnight mark
 * @param FixTime NMEA format fix time (HHMMSS)
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Seconds-based FixTime
 */
fixed
NMEAParser::TimeModify(fixed FixTime, BrokenDateTime &date_time,
                       bool date_available)
{
  assert(!date_available || BrokenDate(date_time).Plausible());

  fixed hours, mins, secs;

  // Calculate Hour
  hours = FixTime / 10000;
  date_time.hour = (int)hours;

  // Calculate Minute
  mins = FixTime / 100;
  mins = mins - fixed(date_time.hour) * 100;
  date_time.minute = (int)mins;

  // Calculate Second
  secs = FixTime - fixed(date_time.hour * 10000 + date_time.minute * 100);
  date_time.second = (int)secs;

  // FixTime is now seconds-based instead of mixed format
  FixTime = secs + fixed(date_time.minute * 60 + date_time.hour * 3600);

  // If (StartDay not yet set and available) set StartDate;
  if (StartDay == -1 && date_available)
    StartDay = date_time.day;

  if (StartDay != -1) {
    if (date_time.day < StartDay)
      // detect change of month (e.g. day=1, startday=31)
      StartDay = date_time.day - 1;

    int day_difference = date_time.day - StartDay;
    if (day_difference > 0)
      // Add seconds to fix time so time doesn't wrap around when
      // going past midnight in UTC
      FixTime += fixed(day_difference * 86400);
  }

  return FixTime;
}

/**
 * Checks whether time has advanced since last call and
 * updates the GPS_info if necessary
 * @param ThisTime Current time
 * @param GPS_INFO GPS_INFO struct to update
 * @return True if time has advanced since last call
 */
bool
NMEAParser::TimeHasAdvanced(fixed ThisTime, NMEA_INFO *GPS_INFO)
{
  if (ThisTime < LastTime) {
    LastTime = ThisTime;
    StartDay = -1; // reset search for the first day
    return false;
  } else {
    GPS_INFO->Time = ThisTime;
    LastTime = ThisTime;
    return true;
  }
}

/**
 * Parses a GSA sentence
 *
 * $--GSA,a,a,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x.x,x.x,x.x*hh
 *
 * Field Number:
 *  1) Selection mode
 *	       M=Manual, forced to operate in 2D or 3D
 *	       A=Automatic, 3D/2D
 *  2) Mode (1 = no fix, 2 = 2D fix, 3 = 3D fix)
 *  3) ID of 1st satellite used for fix
 *  4) ID of 2nd satellite used for fix
 *  ...
 *  14) ID of 12th satellite used for fix
 *  15) PDOP
 *  16) HDOP
 *  17) VDOP
 *  18) checksum
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::GSA(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  line.skip();

  if (line.read(0) == 1)
    GPS_INFO->LocationAvailable.Clear();

  // satellites are in items 4-15 of GSA string (4-15 is 1-indexed)
  for (unsigned i = 0; i < GPS_STATE::MAXSATELLITES; i++)
    GPS_INFO->gps.SatelliteIDs[i] = line.read(0);

  return true;
}

/**
 * Parses a GLL sentence
 *
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
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::GLL(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  fixed ThisTime = TimeModify(line.read(fixed_zero), GPS_INFO->DateTime,
                              GPS_INFO->DateAvailable);

  gpsValid = !NAVWarn(line.read_first_char());

  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return true;

  if (!gpsValid)
    GPS_INFO->LocationAvailable.Clear();
  else if (valid_location)
    GPS_INFO->LocationAvailable.Update(GPS_INFO->Time);

  if (valid_location)
    GPS_INFO->Location = location;

  GPS_INFO->gps.real = real;
#ifdef ANDROID
  GPS_INFO->gps.AndroidInternalGPS = false;
#endif

  return true;
}

/**
 * Parses a RMB sentence
 * (not used anymore)
 *
 * $--RMB,A,x.x,a,c--c,c--c,llll.ll,a,yyyyy.yy,a,x.x,x.x,x.x,A,m,*hh
 *
 * Field Number:
 *  1) Status, A= Active, V = Void
 *  2) Cross Track error - nautical miles
 *  3) Direction to Steer, Left or Right
 *  4) TO Waypoint ID
 *  5) FROM Waypoint ID
 *  6) Destination Waypoint Latitude
 *  7) N or S
 *  8) Destination Waypoint Longitude
 *  9) E or W
 * 10) Range to destination in nautical miles
 * 11) Bearing to destination in degrees True
 * 12) Destination closing velocity in knots
 * 13) Arrival Status, A = Arrival Circle Entered
 * 14) FAA mode indicator (NMEA 2.3 and later)
 * 15) Checksum
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::RMB(gcc_unused NMEAInputLine &line, gcc_unused NMEA_INFO *GPS_INFO)
{
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

/**
 * Parses a RMC sentence
 *
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
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::RMC(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  fixed ThisTime = line.read(fixed_zero);

  gpsValid = !NAVWarn(line.read_first_char());

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  GPS_STATE &gps = GPS_INFO->gps;

  fixed speed;
  bool GroundSpeedAvailable = line.read_checked(speed);
  gps.MovementDetected = GroundSpeedAvailable && speed > fixed_two;

  fixed TrackBearing;
  bool TrackBearingAvailable = line.read_checked(TrackBearing);

  // JMW get date info first so TimeModify is accurate
  if (ReadDate(line, GPS_INFO->DateTime))
    GPS_INFO->DateAvailable = true;

  ThisTime = TimeModify(ThisTime, GPS_INFO->DateTime, GPS_INFO->DateAvailable);

  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return true;

  if (!gpsValid)
    GPS_INFO->LocationAvailable.Clear();
  else if (valid_location)
    GPS_INFO->LocationAvailable.Update(GPS_INFO->Time);

  if (valid_location)
    GPS_INFO->Location = location;

  if (GroundSpeedAvailable) {
    GPS_INFO->GroundSpeed = Units::ToSysUnit(speed, unKnots);
    GPS_INFO->GroundSpeedAvailable.Update(GPS_INFO->Time);
  }

  if (TrackBearingAvailable && gps.MovementDetected) {
    // JMW don't update bearing unless we're moving
    GPS_INFO->TrackBearing = Angle::degrees(TrackBearing).as_bearing();
    GPS_INFO->TrackBearingAvailable.Update(GPS_INFO->Time);
  }

  if (!GGAAvailable) {
    // update SatInUse, some GPS receiver don't emit GGA sentence
    if (!gpsValid)
      gps.SatellitesUsed = 0;
    else
      gps.SatellitesUsed = -1;
  }

  GPS_INFO->gps.real = real;
#ifdef ANDROID
  GPS_INFO->gps.AndroidInternalGPS = false;
#endif

  return true;
}

/**
 * Parses a GGA sentence
 *
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
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::GGA(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  GPS_STATE &gps = GPS_INFO->gps;

  GGAAvailable = true;

  fixed ThisTime = TimeModify(line.read(fixed_zero), GPS_INFO->DateTime,
                              GPS_INFO->DateAvailable);

  GeoPoint location;
  bool valid_location = ReadGeoPoint(line, location);

  gps.FixQuality = line.read(0);
  if (gps.FixQuality != 1 && gps.FixQuality != 2)
    gpsValid = false;

  int nSatellites = min(16, line.read(0));
  if (nSatellites == 0)
    gpsValid = false;

  gps.SatellitesUsed = nSatellites;

  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return true;

  if (!gpsValid)
    GPS_INFO->LocationAvailable.Clear();
  else if (valid_location)
    GPS_INFO->LocationAvailable.Update(GPS_INFO->Time);

  if (valid_location)
    GPS_INFO->Location = location;

  GPS_INFO->gps.real = real;
#ifdef ANDROID
  GPS_INFO->gps.AndroidInternalGPS = false;
#endif

  gps.HDOP = line.read(fixed_zero);

  // VENTA3 CONDOR ALTITUDE
  // "Altitude" should always be GPS Altitude.

  bool altitude_available = ReadAltitude(line, GPS_INFO->GPSAltitude);
  if (altitude_available)
    GPS_INFO->GPSAltitudeAvailable.Update(GPS_INFO->Time);
  else {
    GPS_INFO->GPSAltitude = fixed_zero;
    GPS_INFO->GPSAltitudeAvailable.Clear();
  }

  fixed GeoidSeparation;
  if (ReadAltitude(line, GeoidSeparation)) {
    // No real need to parse this value,
    // but we do assume that no correction is required in this case

    if (!altitude_available) {
      /* Some devices, such as the "LG Incite Cellphone" seem to be
         severely bugged, and report the GPS altitude in the Geoid
         column.  That sucks! */
      GPS_INFO->GPSAltitude = GeoidSeparation;
      GPS_INFO->GPSAltitudeAvailable.Update(GPS_INFO->Time);
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
    if (!HaveCondorDevice()) {
      // JMW TODO really need to know the actual device..
      GeoidSeparation = LookupGeoidSeparation(GPS_INFO->Location);
      GPS_INFO->GPSAltitude -= GeoidSeparation;
    }
  }

  return true;
}

/**
 * Parses a PGRMZ sentence (Garmin proprietary).
 *
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::RMZ(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  //JMW?  RMZAltitude = GPS_INFO->pressure.PressureAltitudeToQNHAltitude(RMZAltitude);

  fixed value;
  if (ReadAltitude(line, value)) {
    // JMW no in-built baro sources, so use this generic one
    if (isFlarm)
      /* FLARM emulates the Garmin $PGRMZ sentence, but emits the
         altitude above 1013.25 hPa - since the don't have a "FLARM"
         device driver, we use the auto-detected "isFlarm" flag
         here */
      GPS_INFO->ProvidePressureAltitude(value);
    else
      GPS_INFO->ProvideBaroAltitudeTrue(value);
  }

  return true;
}

/**
 * Calculates the checksum of the provided NMEA string and
 * compares it to the provided checksum
 * @param String NMEA string
 * @return True if checksum correct
 */
bool
NMEAParser::NMEAChecksum(const char *String)
{
  return ignore_checksum || VerifyNMEAChecksum(String);
}

/**
 * Parses a PTAS1 sentence (Tasman Instruments proprietary).
 *
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool
NMEAParser::PTAS1(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  fixed wnet;
  if (line.read_checked(wnet))
    GPS_INFO->ProvideTotalEnergyVario(Units::ToSysUnit((wnet - fixed(200)) / 10,
                                                       unKnots));

  line.skip(); // average vario +200

  fixed baralt;
  if (line.read_checked(baralt)) {
    baralt = max(fixed_zero, Units::ToSysUnit(baralt - fixed(2000), unFeet));
    GPS_INFO->ProvidePressureAltitude(baralt);
  }

  fixed vtas;
  if (line.read_checked(vtas))
    GPS_INFO->ProvideTrueAirspeed(Units::ToSysUnit(vtas, unKnots));

  return true;
}

/**
 * Parses a PFLAU sentence
 * (Operating status and priority intruder and obstacle data)
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 * @see http://flarm.com/support/manual/FLARM_DataportManual_v5.00E.pdf
 */
bool
NMEAParser::PFLAU(NMEAInputLine &line, FLARM_STATE &flarm, fixed Time)
{
  flarm.available.Update(Time);
  isFlarm = true;

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  flarm.rx = line.read(0);
  flarm.tx = line.read(0);
  flarm.gps = line.read(0);
  line.skip();
  flarm.alarm_level = line.read(0);

  return true;
}

/**
 * Parses a PFLAA sentence
 * (Data on other moving objects around)
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 * @see http://flarm.com/support/manual/FLARM_DataportManual_v5.00E.pdf
 */
bool
NMEAParser::PFLAA(NMEAInputLine &line, NMEA_INFO *GPS_INFO)
{
  FLARM_STATE &flarm = GPS_INFO->flarm;

  isFlarm = true;

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  FLARM_TRAFFIC traffic;
  traffic.AlarmLevel = line.read(0);

  fixed value;
  bool stealth = false;

  if (!line.read_checked(value))
    // Relative North is required !
    return true;
  traffic.RelativeNorth = value;

  if (!line.read_checked(value))
    // Relative East is required !
    return true;
  traffic.RelativeEast = value;

  if (!line.read_checked(value))
    // Relative Altitude is required !
    return true;
  traffic.RelativeAltitude = value;

  traffic.IDType = line.read(0);

  // 5 id, 6 digit hex
  char id_string[16];
  line.read(id_string, 16);
  traffic.ID.parse(id_string, NULL);

  traffic.track_received = line.read_checked(value);
  if (!traffic.track_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.TrackBearing = Angle::native(fixed_zero);
  } else
    traffic.TrackBearing = Angle::degrees(value);

  traffic.turn_rate_received = line.read_checked(value);
  if (!traffic.turn_rate_received) {
    // Field is empty in stealth mode
    traffic.TurnRate = fixed_zero;
  } else
    traffic.TurnRate = value;

  traffic.speed_received = line.read_checked(value);
  if (!traffic.speed_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.Speed = fixed_zero;
  } else
    traffic.Speed = value;

  traffic.climb_rate_received = line.read_checked(value);
  if (!traffic.climb_rate_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.ClimbRate = fixed_zero;
  } else
    traffic.ClimbRate = value;

  traffic.Stealth = stealth;

  traffic.Type = (FLARM_TRAFFIC::AircraftType)line.read(0);

  FLARM_TRAFFIC *flarm_slot = flarm.FindTraffic(traffic.ID);
  if (flarm_slot == NULL) {
    flarm_slot = flarm.AllocateTraffic();
    if (flarm_slot == NULL)
      // no more slots available
      return true;

    flarm_slot->Clear();
    flarm_slot->ID = traffic.ID;

    flarm.NewTraffic = true;
  }

  // set time of fix to current time
  flarm_slot->Valid.Update(GPS_INFO->Time);

  flarm_slot->Update(traffic);

  return true;
}
