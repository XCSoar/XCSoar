/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Device/Parser.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include <math.h>
#include "Audio/VarioSound.h"
#include "Device/device.h"
#include "Device/Geoid.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "NMEA/Info.h"
#include "NMEA/Checksum.h"
#include "StringUtil.hpp"

#include <ctype.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

#ifdef FLARM_AVERAGE
#include "FlarmCalculations.h"
FlarmCalculations flarmCalculations;
#endif

#define MAX_NMEA_LEN	90
#define MAX_NMEA_PARAMS 18

static double EastOrWest(double in, TCHAR EoW);
static double NorthOrSouth(double in, TCHAR NoS);
//static double LeftOrRight(double in, TCHAR LoR);
static double MixedFormatToDegrees(double mixed);
static int NAVWarn(TCHAR c);

NMEAParser nmeaParser1;
NMEAParser nmeaParser2;


int NMEAParser::StartDay = -1;

/**
 * Constructor of the NMEAParser class
 * @return NMEAParser object
 */
NMEAParser::NMEAParser() {
  _Reset();
}

/**
 * Resets the NMEAParser
 */
void NMEAParser::_Reset(void) {
  nSatellites = 0;
  gpsValid = false;
  isFlarm = false;
  activeGPS = true;
  GGAAvailable = false;
  RMZAvailable = false;
  RMZAltitude = 0;
  RMAAvailable = false;
  RMAAltitude = 0;
  LastTime = 0;
}

/**
 * Resets both NMEAParsers (Port1 + Port2)
 */
void NMEAParser::Reset(void) {
  // clear status
  nmeaParser1._Reset();
  nmeaParser2._Reset();
}

/**
 * Checks which port has valid GPS information and activates it
 */
void NMEAParser::UpdateMonitor(void)
{
  // does anyone have GPS?
  if (nmeaParser1.gpsValid || nmeaParser2.gpsValid) {
    if (nmeaParser1.gpsValid && nmeaParser2.gpsValid) {
      // both valid, just use first
      nmeaParser2.activeGPS = false;
      nmeaParser1.activeGPS = true;
    } else {
      nmeaParser1.activeGPS = nmeaParser1.gpsValid;
      nmeaParser2.activeGPS = nmeaParser2.gpsValid;
    }
  } else {
    // assume device 1 is active
    nmeaParser2.activeGPS = false;
    nmeaParser1.activeGPS = true;
  }
}

/**
 * Redirects parsing of the String into GPS_INFO to the right device parser
 * @param device 0 or 1, depending on the port used
 * @param String NMEA string
 * @param GPS_INFO GPS_INFO struct that will be updated
 * @return Parsing success
 */
bool NMEAParser::ParseNMEAString(int device, const TCHAR *String,
    NMEA_INFO *GPS_INFO) {
  switch (device) {
  case 0:
    return nmeaParser1.ParseNMEAString_Internal(String, GPS_INFO);
  case 1:
    return nmeaParser2.ParseNMEAString_Internal(String, GPS_INFO);
  };
  return false;
}

/**
 * Copies a provided string into the supplied buffer, terminates on
 * the checksum separator, splits into an array of parameters,
 * and returns the number of parameters found.
 * @param src Source string
 * @param dst Buffer string
 * @param arr Parameter array
 * @param sz Parameter array size
 * @see ValidateAndExtract
 * @return Number of extracted parameters
 */
size_t NMEAParser::ExtractParameters(const TCHAR *src, TCHAR *dst,
                                     const TCHAR **arr, size_t sz)
{
  TCHAR c, *p;
  size_t i = 0;

  _tcscpy(dst, src);
  p = _tcschr(dst, _T('*'));
  if (p)
    *p = _T('\0');

  p = dst;
  do {
    arr[i++] = p;
    p = _tcschr(p, _T(','));
    if (!p)
      break;
    c = *p;
    *p++ = _T('\0');
  } while (i != sz && c != _T('\0'));

  return i;
}

/**
 * Same as ExtractParameters, but also validate the length of
 * the string and the NMEA checksum.
 * @param src Source string
 * @param dst Buffer string
 * @param dstsz Buffer size
 * @param arr Parameter array
 * @param arrsz Parameter array size
 * @see ExtractParameters
 * @return Number of extracted parameters
 */
size_t NMEAParser::ValidateAndExtract(const TCHAR *src, TCHAR *dst,
    size_t dstsz, const TCHAR **arr, size_t arrsz) {
  // len = Length of the source string
  int len = _tcslen(src);

  // if (len <= 6 characters  or  len >= buffer size) cancel method;
  if (len <= 6 || len >= (int) dstsz)
    return 0;

  // if (checksum of the source string is incorrect) cancel method;
  if (!NMEAChecksum(src))
    return 0;

  return ExtractParameters(src, dst, arr, arrsz);
}

/**
 * Parses a provided NMEA String into a GPS_INFO struct
 * @param String NMEA string
 * @param GPS_INFO GPS_INFO output struct
 * @return Parsing success
 */
bool NMEAParser::ParseNMEAString_Internal(const TCHAR *String,
    NMEA_INFO *GPS_INFO) {
  TCHAR ctemp[MAX_NMEA_LEN];
  const TCHAR *params[MAX_NMEA_PARAMS];
  size_t n_params;

  n_params = ValidateAndExtract(String, ctemp, MAX_NMEA_LEN, params,
      MAX_NMEA_PARAMS);
  // if (not enough parameters  or  first parameter invalid) cancel method;
  if (n_params < 1 || params[0][0] != '$')
    return false;

  // if (logging enabled) log;
  if (EnableLogNMEA)
    LogNMEA(String);

  // if (proprietary sentence) ...
  if (params[0][1] == 'P') {
    // Airspeed and vario sentence
    if (_tcscmp(params[0] + 1, _T("PTAS1")) == 0) {
      return PTAS1(&String[7], params + 1, n_params, GPS_INFO);
    }

    // FLARM sentences
    if (_tcscmp(params[0] + 1, _T("PFLAA")) == 0) {
      return PFLAA(&String[7], params + 1, n_params, GPS_INFO);
    }

    if (_tcscmp(params[0] + 1, _T("PFLAU")) == 0) {
      return PFLAU(&String[7], params + 1, n_params, GPS_INFO);
    }

    // Garmin altitude sentence
    if (_tcscmp(params[0] + 1, _T("PGRMZ")) == 0) {
      return RMZ(&String[7], params + 1, n_params, GPS_INFO);
    }
    return false;
  }

  if (_tcscmp(params[0] + 3, _T("GSA")) == 0) {
    return GSA(&String[7], params + 1, n_params, GPS_INFO);
  }
  if (_tcscmp(params[0] + 3, _T("GLL")) == 0) {
    //    return GLL(&String[7], params + 1, n_params, GPS_INFO);
    return false;
  }
  if (_tcscmp(params[0] + 3, _T("RMB")) == 0) {
    //return RMB(&String[7], params + 1, n_params, GPS_INFO);
    return false;
  }
  if (_tcscmp(params[0] + 3, _T("RMC")) == 0) {
    return RMC(&String[7], params + 1, n_params, GPS_INFO);
  }
  if (_tcscmp(params[0] + 3, _T("GGA")) == 0) {
    return GGA(&String[7], params + 1, n_params, GPS_INFO);
  }

  return false;
}

/**
 * Extracts a certain parameter out of a NMEA string
 * @param Source NMEA string
 * @param Destination Buffer string
 * @param DesiredFieldNumber Parameter id
 * @see ExtractParameters
 */
void NMEAParser::ExtractParameter(const TCHAR *Source, TCHAR *Destination,
    int DesiredFieldNumber) {
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength = _tcslen(Source);
  const TCHAR *sptr = Source;
  const TCHAR *eptr = Source + StringLength;

  if (!Destination)
    return;

  while ((CurrentFieldNumber < DesiredFieldNumber) && (sptr < eptr)) {
    if (*sptr == ',' || *sptr == '*') {
      CurrentFieldNumber++;
    }
    ++sptr;
  }

  Destination[0] = '\0'; // set to blank in case it's not found..

  if (CurrentFieldNumber == DesiredFieldNumber) {
    while ((sptr < eptr) && (*sptr != ',') && (*sptr != '*') && (*sptr != '\0')) {
      Destination[dest_index] = *sptr;
      ++sptr;
      ++dest_index;
    }
    Destination[dest_index] = '\0';
  }
}

/**
 * Converts a given double and 'E' and 'W' to the appropriate signed double
 * @param in Input value
 * @param EoW Input direction
 * @return Signed value
 */
double EastOrWest(double in, TCHAR EoW)
{
  if(EoW == 'W')
    return -in;
  else
    return in;
}

/**
 * Converts a given double and 'N' and 'S' to the appropriate signed double
 * @param in Input value
 * @param NoS Input direction
 * @return Signed value
 */
double NorthOrSouth(double in, TCHAR NoS)
{
  if(NoS == 'S')
    return -in;
  else
    return in;
}

/*
double LeftOrRight(double in, TCHAR LoR)
{
  if(LoR == 'L')
    return -in;
  else
    return in;
}
*/

/**
 * Parses whether the given character (GPS status) should create a navigational warning
 * @param c GPS status
 * @return True if GPS fix not found or invalid
 */
int NAVWarn(TCHAR c)
{
  if(c=='A')
    return false;
  else
    return true;
}

/**
 * Parses an altitude into the metric system if necessary
 * @param value Altitude value
 * @param format Altitude unit
 * @return Altitude in meters
 */
double NMEAParser::ParseAltitude(const TCHAR *value, const TCHAR *format)
{
  double alt = _tcstod(value, NULL);

  if (format[0] == _T('f') || format[0] == _T('F'))
    alt /= TOFEET;

  return alt;
}

/**
 * Converts the mixed lat/lon format to pure degrees
 *
 * Example:
 * 3845.587
 * => 38 + 45.587/60
 * = 38.75978333 degrees
 * @param mixed Mixed formated string
 * @return Degrees
 */
double MixedFormatToDegrees(double mixed)
{
  double mins, degrees;

  degrees = (int) (mixed/100);
  mins = (mixed - degrees*100)/60;

  return degrees+mins;
}

/**
 * Calculates a seconds-based FixTime and corrects it
 * in case over passing the UTC midnight mark
 * @param FixTime NMEA format fix time (HHMMSS)
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Seconds-based FixTime
 */
double NMEAParser::TimeModify(double FixTime, NMEA_INFO* GPS_INFO)
{
  double hours, mins, secs;

  // Calculate Hour
  hours = FixTime / 10000;
  GPS_INFO->Hour = (int)hours;

  // Calculate Minute
  mins = FixTime / 100;
  mins = mins - (GPS_INFO->Hour*100);
  GPS_INFO->Minute = (int)mins;

  // Calculate Second
  secs = FixTime - (GPS_INFO->Hour*10000) - (GPS_INFO->Minute*100);
  GPS_INFO->Second = (int)secs;

  // FixTime is now seconds-based instead of mixed format
  FixTime = secs + (GPS_INFO->Minute*60) + (GPS_INFO->Hour*3600);

  // If (StartDay not yet set and available) set StartDate;
  if ((StartDay== -1) && (GPS_INFO->Day != 0)) {
    StartDay = GPS_INFO->Day;
  }
  if (StartDay != -1) {
    if (GPS_INFO->Day < StartDay) {
      // detect change of month (e.g. day=1, startday=31)
      StartDay = GPS_INFO->Day-1;
    }
    int day_difference = GPS_INFO->Day-StartDay;
    if (day_difference>0) {
      // Add seconds to fix time so time doesn't wrap around when
      // going past midnight in UTC
      FixTime += day_difference * 86400;
    }
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
bool NMEAParser::TimeHasAdvanced(double ThisTime, NMEA_INFO *GPS_INFO) {
  if(ThisTime < LastTime) {
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
bool NMEAParser::GSA(const TCHAR *String,
                     const TCHAR **params, size_t nparams, NMEA_INFO *GPS_INFO)
{
  int iSatelliteCount =0;

  if (GPS_INFO->Replay) {
    return true;
  }

  // satellites are in items 4-15 of GSA string (4-15 is 1-indexed)
  // but 1st item in string is not passed, so start at item 3
  for (int i = 0; i < MAXSATELLITES; i++)
  {
    if (3+i < (int) nparams) {
      GPS_INFO->SatelliteIDs[i] = (int)(_tcstod(params[2+i], NULL)); // 2 because params is 0-index
      if (GPS_INFO->SatelliteIDs[i] > 0)
	iSatelliteCount ++;
    }
  }

  GSAAvailable = true;
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
bool NMEAParser::GLL(const TCHAR *String,
                     const TCHAR **params, size_t nparams, NMEA_INFO *GPS_INFO)
{
  gpsValid = !NAVWarn(params[5][0]);

  if (!activeGPS)
    return true;

  if (GPS_INFO->Replay) {
    // block actual GPS signal
    return true;
  }

  GPS_INFO->NAVWarning = !gpsValid;


  double ThisTime = TimeModify(_tcstod(params[4], NULL), GPS_INFO);
  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return false;

  double tmplat;
  double tmplon;

  tmplat = MixedFormatToDegrees(_tcstod(params[0], NULL));
  tmplat = NorthOrSouth(tmplat, params[1][0]);

  tmplon = MixedFormatToDegrees(_tcstod(params[2], NULL));
  tmplon = EastOrWest(tmplon,params[3][0]);

  if (!((tmplat == 0.0) && (tmplon == 0.0))) {
    GPS_INFO->Location.Latitude = tmplat;
    GPS_INFO->Location.Longitude = tmplon;
  } else {
	  // QUESTION TB: why conditional?
  }
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
bool NMEAParser::RMB(const TCHAR *String,
                     const TCHAR **params, size_t nparams, NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;
  (void)String;
  (void)params;
  (void)nparams;

  /* we calculate all this stuff now
  TCHAR ctemp[MAX_NMEA_LEN];

  GPS_INFO->NAVWarning = NAVWarn(params[0][0]);

  GPS_INFO->CrossTrackError = NAUTICALMILESTOMETRES * _tcstod(params[1], NULL);
  GPS_INFO->CrossTrackError = LeftOrRight(GPS_INFO->CrossTrackError,params[2][0]);

  _tcscpy(ctemp, params[4]);
  ctemp[WAY_POINT_ID_SIZE] = '\0';
  _tcscpy(GPS_INFO->WaypointID,ctemp);

  GPS_INFO->WaypointDistance = NAUTICALMILESTOMETRES * _tcstod(params[9], NULL);
  GPS_INFO->WaypointBearing = _tcstod(params[10], NULL);
  GPS_INFO->WaypointSpeed = KNOTSTOMETRESSECONDS * _tcstod(params[11], NULL);
  */

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
bool NMEAParser::RMC(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{
  TCHAR *Stop;

  gpsValid = !NAVWarn(params[1][0]);

  GPS_INFO->Connected = 2;

  if (!activeGPS)
    return true;

  double speed = _tcstod(params[6], NULL);

  if (speed>2.0) {
    GPS_INFO->MovementDetected = true;
    if (GPS_INFO->Replay) {
      return true;
    }
  } else {
    GPS_INFO->MovementDetected = false;
    if (GPS_INFO->Replay) {
      // block actual GPS signal if not moving and a log is being replayed
      return true;
    }
  }

  GPS_INFO->NAVWarning = !gpsValid;

  // JMW get date info first so TimeModify is accurate
  TCHAR date_buffer[9];
  _tcsncpy(date_buffer, params[8], sizeof(date_buffer) - 1);
  date_buffer[sizeof(date_buffer) - 1] = 0;

  GPS_INFO->Year = _tcstol(&date_buffer[4], &Stop, 10) + 2000;
  date_buffer[4] = '\0';
  GPS_INFO->Month = _tcstol(&date_buffer[2], &Stop, 10);
  date_buffer[2] = '\0';
  GPS_INFO->Day = _tcstol(&date_buffer[0], &Stop, 10);

  double ThisTime = TimeModify(_tcstod(params[0], NULL), GPS_INFO);
  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return false;

  double tmplat;
  double tmplon;

  tmplat = MixedFormatToDegrees(_tcstod(params[2], NULL));
  tmplat = NorthOrSouth(tmplat, params[3][0]);

  tmplon = MixedFormatToDegrees(_tcstod(params[4], NULL));
  tmplon = EastOrWest(tmplon,params[5][0]);

  if (!((tmplat == 0.0) && (tmplon == 0.0))) {
    GPS_INFO->Location.Latitude = tmplat;
    GPS_INFO->Location.Longitude = tmplon;
  }

  GPS_INFO->Speed = KNOTSTOMETRESSECONDS * speed;

  if (GPS_INFO->Speed>1.0) {
    // JMW don't update bearing unless we're moving
    GPS_INFO->TrackBearing = AngleLimit360(_tcstod(params[7], NULL));
  }

  if (!GPS_INFO->Replay) {
    if (RMZAvailable) {
      // JMW changed from Altitude to BaroAltitude
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMZAltitude;
    } else if (RMAAvailable) {
      // JMW changed from Altitude to BaroAltitude
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMAAltitude;
    }
  }
  if (!GGAAvailable) {
    // update SatInUse, some GPS receiver don't emit GGA sentence
    if (!gpsValid) {
      GPS_INFO->SatellitesUsed = 0;
    } else {
      GPS_INFO->SatellitesUsed = -1;
    }
  }

  // say we are updated every time we get this,
  // so infoboxes get refreshed if GPS connected
  TriggerGPSUpdate();

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
bool NMEAParser::GGA(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{

  if (GPS_INFO->Replay) {
    return true;
  }

  GGAAvailable = true;

  nSatellites = (int)(min(16.0, _tcstod(params[6], NULL)));
  if (nSatellites==0) {
    gpsValid = false;
  }

  if (!activeGPS)
    return true;

  GPS_INFO->SatellitesUsed = (int)(min(16.0, _tcstod(params[6], NULL)));

  double ThisTime = TimeModify(_tcstod(params[0], NULL), GPS_INFO);
  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return false;

  double tmplat;
  double tmplon;

  tmplat = MixedFormatToDegrees(_tcstod(params[1], NULL));
  tmplat = NorthOrSouth(tmplat, params[2][0]);

  tmplon = MixedFormatToDegrees(_tcstod(params[3], NULL));
  tmplon = EastOrWest(tmplon,params[4][0]);

  if (!((tmplat == 0.0) && (tmplon == 0.0))) {
    GPS_INFO->Location.Latitude = tmplat;
    GPS_INFO->Location.Longitude = tmplon;
  }

  if(RMZAvailable)
    {
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMZAltitude;
    }
  else if(RMAAvailable)
    {
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMAAltitude;
    }

  // VENTA3 CONDOR ALTITUDE
  // "Altitude" should always be GPS Altitude.
  GPS_INFO->GPSAltitude = ParseAltitude(params[8], params[9]);

  double GeoidSeparation;
  if (!string_is_empty(params[10])) {
    // No real need to parse this value,
    // but we do assume that no correction is required in this case
    GeoidSeparation = ParseAltitude(params[10], params[11]);
  } else {
    // need to estimate Geoid Separation internally (optional)
    // FLARM uses MSL altitude
    //
    // Some others don't.
    //
    // If the separation doesn't appear in the sentence,
    // we can assume the GPS unit is giving ellipsoid height
    //
    if (!devIsCondor(devA())) {
      // JMW TODO really need to know the actual device..
      GeoidSeparation = LookupGeoidSeparation(GPS_INFO->Location.Latitude,
					      GPS_INFO->Location.Longitude);
      GPS_INFO->GPSAltitude -= GeoidSeparation;
    }
  }

  return true;
}

/**
 * Parses a RMZ sentence
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool NMEAParser::RMZ(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{
  RMZAltitude = ParseAltitude(params[0], params[1]);
  //JMW?  RMZAltitude = GPS_INFO->pressure.AltitudeToQNHAltitude(RMZAltitude);
  RMZAvailable = true;

  if (!devHasBaroSource()) {
    // JMW no in-built baro sources, so use this generic one
    if (!GPS_INFO->Replay) {
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMZAltitude;
    }
  }

  return false;
}

/**
 * Parses a RMA sentence
 * (not in use and maybe faulty(?))
 *
 * $--RMA,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,x.x,x.x,x.x,a*hh
 *
 * Field Number:
 *  1) Blink Warning
 *  2) Latitude
 *  3) N or S
 *  4) Longitude
 *  5) E or W
 *  6) Time Difference A, uS
 *  7) Time Difference B, uS
 *  8) Speed Over Ground, Knots
 *  9) Track Made Good, degrees true
 * 10) Magnetic Variation, degrees
 * 11) E or W
 * 12) Checksum
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool NMEAParser::RMA(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;

  RMAAltitude = ParseAltitude(params[0], params[1]);
  //JMW?  RMAAltitude = GPS_INFO->pressure.AltitudeToQNHAltitude(RMAAltitude);
  RMAAvailable = true;
  GPS_INFO->BaroAltitudeAvailable = true;

  if (!devHasBaroSource()) {
    if (!GPS_INFO->Replay) {
      // JMW no in-built baro sources, so use this generic one
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMAAltitude;
    }
  }

  return false;
}

/**
 * Calculates the checksum of the provided NMEA string and
 * compares it to the provided checksum
 * @param String NMEA string
 * @return True if checksum correct
 */
bool NMEAParser::NMEAChecksum(const TCHAR *String)
{
  unsigned char ReadCheckSum, CalcCheckSum;
  TCHAR c1,c2;
  unsigned char v1 = 0,v2 = 0;
  const TCHAR *pEnd;

  pEnd = _tcschr(String,'*');
  if(pEnd == NULL)
    return false;

  if(_tcslen(pEnd)<3)
    return false;

  c1 = pEnd[1], c2 = pEnd[2];

  //  iswdigit('0'); // what's this for?

  if(_istdigit(c1))
    v1 = (unsigned char)(c1 - '0');
  if(_istdigit(c2))
    v2 = (unsigned char)(c2 - '0');
  if(_istalpha(c1))
    v1 = (unsigned char)(c1 - 'A' + 10);
  if(_istalpha(c2))
    v2 = (unsigned char)(c2 - 'A' + 10);

  ReadCheckSum = (unsigned char)((v1<<4) + v2);
  CalcCheckSum = ::NMEAChecksum(String + 1, pEnd - String - 1);

  if(CalcCheckSum == ReadCheckSum)
    return true;
  else
    return false;
}

/**
 * Parses a PTAS1 sentence
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 */
bool NMEAParser::PTAS1(const TCHAR *String,
                       const TCHAR **params, size_t nparams,
                       NMEA_INFO *GPS_INFO)
{
  double wnet,baralt,vtas;

  wnet = (_tcstod(params[0], NULL) - 200) / (10 * TOKNOTS);
  baralt = max(0.0, (_tcstod(params[2], NULL) - 2000) / TOFEET);
  vtas = _tcstod(params[3], NULL) / TOKNOTS;

  GPS_INFO->AirspeedAvailable = true;
  GPS_INFO->TrueAirspeed = vtas;
  GPS_INFO->VarioAvailable = true;
  GPS_INFO->Vario = wnet;
  GPS_INFO->BaroAltitudeAvailable = true;
  GPS_INFO->BaroAltitude = GPS_INFO->pressure.AltitudeToQNHAltitude(baralt);
  GPS_INFO->IndicatedAirspeed = vtas/GPS_INFO->pressure.AirDensityRatio(baralt);

  TriggerVarioUpdate();

  return false;
}


#include "InputEvents.h"

static double FLARM_NorthingToLatitude = 0.0;
static double FLARM_EastingToLongitude = 0.0;

/**
 * Parses a PFLAU sentence
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 * @see http://flarm.com/support/manual/FLARM_DataportManual_v4.06E.pdf
 */
bool NMEAParser::PFLAU(const TCHAR *String,
                       const TCHAR **params, size_t nparams,
                       NMEA_INFO *GPS_INFO)
{
  static int old_flarm_rx = 0;

  GPS_INFO->FLARM_Available = true;
  isFlarm = true;

  // calculate relative east and north projection to lat/lon

  double delta_lat = 0.01;
  double delta_lon = 0.01;

  GEOPOINT plat = GPS_INFO->Location; plat.Latitude+= delta_lat;
  GEOPOINT plon = GPS_INFO->Location; plon.Longitude+= delta_lon;

  double dlat = Distance(GPS_INFO->Location, plat);
  double dlon = Distance(GPS_INFO->Location, plon);

  if ((fabs(dlat)>0.0)&&(fabs(dlon)>0.0)) {
    FLARM_NorthingToLatitude = delta_lat / dlat;
    FLARM_EastingToLongitude = delta_lon / dlon;
  } else {
    FLARM_NorthingToLatitude=0.0;
    FLARM_EastingToLongitude=0.0;
  }

  _stscanf(String,
	  _T("%hu,%hu,%hu,%hu"),
	  &GPS_INFO->FLARM_RX,
	  &GPS_INFO->FLARM_TX,
	  &GPS_INFO->FLARM_GPS,
	  &GPS_INFO->FLARM_AlarmLevel);

  // process flarm updates

  if ((GPS_INFO->FLARM_RX) && (old_flarm_rx==0)) {
    // traffic has appeared..
    InputEvents::processGlideComputer(GCE_FLARM_TRAFFIC);
  }
  if ((GPS_INFO->FLARM_RX==0) && (old_flarm_rx)) {
    // traffic has disappeared..
    InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);
  }
  // TODO feature: add another event for new traffic.

  old_flarm_rx = GPS_INFO->FLARM_RX;

  return false;
}

/**
 * Finds existing or new slot of given FLARM id in FLARM_Traffic array
 * @param GPS_INFO Pointer to the basic GPS info struct
 * @param Id FLARM id
 * @return Existing or new slot, -1 if buffer is full
 */
int FLARM_FindSlot(NMEA_INFO *GPS_INFO, long Id)
{
  int i;
  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    // find position in existing slot
    if (Id==GPS_INFO->FLARM_Traffic[i].ID) {
      return i;
    }
    // find old empty slot
  }

  // not found, so try to find an empty slot
  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (GPS_INFO->FLARM_Traffic[i].ID==0) {
      // this is a new target
      GPS_INFO->NewTraffic = true;
      return i;
    }
  }

  // still not found and no empty slots left, buffer is full
  return -1;
}

/**
 * Parses a PFLAA sentence
 * @param String Input string
 * @param params Parameter array
 * @param nparams Number of parameters
 * @param GPS_INFO GPS_INFO struct to parse into
 * @return Parsing success
 * @see http://flarm.com/support/manual/FLARM_DataportManual_v4.06E.pdf
 */
bool NMEAParser::PFLAA(const TCHAR *String,
                       const TCHAR **params, size_t nparams,
                       NMEA_INFO *GPS_INFO)
{
  int flarm_slot = 0;

  isFlarm = true;

  // 5 id, 6 digit hex
  long ID;
  _stscanf(params[5], _T("%lx"), &ID);
  //  unsigned long uID = ID;

  flarm_slot = FLARM_FindSlot(GPS_INFO, ID);
  if (flarm_slot<0) {
    // no more slots available,
    return false;
  }

  // set time of fix to current time
  GPS_INFO->FLARM_Traffic[flarm_slot].Time_Fix = GPS_INFO->Time;

  _stscanf(String,
	  _T("%hu,%lf,%lf,%lf,%hu,%lx,%lf,%lf,%lf,%lf,%hu"),
	  &GPS_INFO->FLARM_Traffic[flarm_slot].AlarmLevel, // unsigned short 0
	  &GPS_INFO->FLARM_Traffic[flarm_slot].RelativeNorth, // double?     1
	  &GPS_INFO->FLARM_Traffic[flarm_slot].RelativeEast, // double?      2
	  &GPS_INFO->FLARM_Traffic[flarm_slot].RelativeAltitude, // double   3
	  &GPS_INFO->FLARM_Traffic[flarm_slot].IDType, // unsigned short     4
	  &GPS_INFO->FLARM_Traffic[flarm_slot].ID, // 6 char hex
	  &GPS_INFO->FLARM_Traffic[flarm_slot].TrackBearing, // double       6
	  &GPS_INFO->FLARM_Traffic[flarm_slot].TurnRate, // double           7
	  &GPS_INFO->FLARM_Traffic[flarm_slot].Speed, // double              8
	  &GPS_INFO->FLARM_Traffic[flarm_slot].ClimbRate, // double          9
	  &GPS_INFO->FLARM_Traffic[flarm_slot].Type); // unsigned short     10

  // 1 relativenorth, meters
  GPS_INFO->FLARM_Traffic[flarm_slot].Location.Latitude =
    GPS_INFO->FLARM_Traffic[flarm_slot].RelativeNorth
    *FLARM_NorthingToLatitude + GPS_INFO->Location.Latitude;

  // 2 relativeeast, meters
  GPS_INFO->FLARM_Traffic[flarm_slot].Location.Longitude =
    GPS_INFO->FLARM_Traffic[flarm_slot].RelativeEast
    *FLARM_EastingToLongitude + GPS_INFO->Location.Longitude;

  // alt
  GPS_INFO->FLARM_Traffic[flarm_slot].Altitude =
    GPS_INFO->FLARM_Traffic[flarm_slot].RelativeAltitude +
    GPS_INFO->GPSAltitude;

#ifdef FLARM_AVERAGE
  GPS_INFO->FLARM_Traffic[flarm_slot].Average30s =
    flarmCalculations.Average30s(
				 GPS_INFO->FLARM_Traffic[flarm_slot].ID,
				 GPS_INFO->Time,
				 GPS_INFO->FLARM_Traffic[flarm_slot].Altitude);
#endif

  // QUESTION TB: never returns true?!
  return false;
}

void NMEAParser::TestRoutine(NMEA_INFO *GPS_INFO) {
	// QUESTION TB: should be moved to test folder!?
#ifndef NDEBUG
#ifndef GNAV
  static int i=90;
  static TCHAR t1[] = _T("1,1,1,1");
  static TCHAR t2[] = _T("1,300,500,220,2,DD927B,0,-4.5,30,-1.4,1");
  static TCHAR t3[] = _T("0,0,1200,50,2,DD9146,270,-4.5,30,-1.4,1");
  //  static TCHAR b50[] = _T("0,.1,.0,0,0,1.06,0,-222");
  //  static TCHAR t4[] = _T("-3,500,1024,50");

  //  nmeaParser1.ParseNMEAString_Internal(_T("$PTAS1,201,200,02583,000*2A"), GPS_INFO);
  //  nmeaParser1.ParseNMEAString_Internal(_T("$GPRMC,082430.00,A,3744.09096,S,14426.16069,E,0.520294.90,301207,,,A*77"), GPS_INFO);
  //  nmeaParser1.ParseNMEAString_Internal(_T("$GPGGA,082430.00,3744.09096,S,1426.16069,E,1,08,1.37,157.6,M,-4.9,M,,*5B"), GPS_INFO);

  i++;

  if (i>100) {
    i=0;
  }
  if (i<50) {
    GPS_INFO->FLARM_Available = true;
    TCHAR ctemp[MAX_NMEA_LEN];
    const TCHAR *params[MAX_NMEA_PARAMS];
    size_t nr;
    nr = nmeaParser1.ExtractParameters(t1, ctemp, params, MAX_NMEA_PARAMS);
    nmeaParser1.PFLAU(t1, params, nr, GPS_INFO);
    nr = nmeaParser1.ExtractParameters(t2, ctemp, params, MAX_NMEA_PARAMS);
    nmeaParser1.PFLAA(t2, params, nr, GPS_INFO);
    nr = nmeaParser1.ExtractParameters(t3, ctemp, params, MAX_NMEA_PARAMS);
    nmeaParser1.PFLAA(t3, params, nr, GPS_INFO);
  }
#endif
#endif
}

bool EnableLogNMEA = false;
HANDLE nmeaLogFile = INVALID_HANDLE_VALUE;

/**
 * Logs NMEA string to log file
 * @param text
 */
void LogNMEA(const TCHAR* text) {

  if (!EnableLogNMEA) {
    if (nmeaLogFile != INVALID_HANDLE_VALUE) {
       CloseHandle(nmeaLogFile);
       nmeaLogFile = INVALID_HANDLE_VALUE;
    }
    return;
  }

  DWORD dwBytesRead;

  if (nmeaLogFile == INVALID_HANDLE_VALUE) {
    nmeaLogFile = CreateFile(_T("\\SD Card\\xcsoar-nmea.log"),
			     GENERIC_WRITE, FILE_SHARE_WRITE,
			     NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  }

  WriteFile(nmeaLogFile, text, _tcslen(text)*sizeof(TCHAR), &dwBytesRead,
	    (OVERLAPPED *)NULL);
}

/**
 * Returns whether the given device is a FLARM unit
 * @param device Device id (0 or 1)
 * @return True if device is a FLARM unit
 */
bool NMEAParser::PortIsFlarm(int device) {
  switch (device) {
  case 0:
    return nmeaParser1.isFlarm;
  case 1:
    return nmeaParser2.isFlarm;
  default:
    return false;
  };
}
