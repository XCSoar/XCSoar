/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Parser.h"
#include "XCSoar.h"
#include "Utils.h"
#include "externs.h"
#include "Audio/VarioSound.h"
#include "Logger.h"
#include "GaugeFLARM.h"
#include "Device/device.h"
#include "Device/Geoid.h"
//#include "FlarmIdFile.h"
#include "TeamCodeCalculation.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Pressure.h"
#include "Math/Units.h"

#include <ctype.h>
#include <stdlib.h>

#ifdef FLARM_AVERAGE
#include "FlarmCalculations.h"
FlarmCalculations flarmCalculations;
#endif

#ifdef __MINGW32__
#ifndef max
#define max(x, y)   (x > y ? x : y)
#define min(x, y)   (x < y ? x : y)
#endif
#endif

extern bool EnableCalibration;

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

NMEAParser::NMEAParser() {
  _Reset();
}

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

void NMEAParser::Reset(void) {

  // clear status
  nmeaParser1._Reset();
  nmeaParser2._Reset();

  // trigger updates
  TriggerGPSUpdate();
  TriggerVarioUpdate();
}


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


bool NMEAParser::ParseNMEAString(int device,
                                 const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  switch (device) {
  case 0:
    return nmeaParser1.ParseNMEAString_Internal(String, GPS_INFO);
  case 1:
    return nmeaParser2.ParseNMEAString_Internal(String, GPS_INFO);
  };
  return false;
}


/*
 * Copy a provided string into the supplied buffer, terminate on
 * the checksum separator, split into an array of parameters,
 * and return the number of parameters found.
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


/*
 * Same as ExtractParameters, but also validate the length of
 * the string and the NMEA checksum.
 */
size_t NMEAParser::ValidateAndExtract(const TCHAR *src, TCHAR *dst, size_t dstsz,
                                      const TCHAR **arr, size_t arrsz)
{
  int len = _tcslen(src);

  if (len <= 6 || len >= (int) dstsz)
    return 0;
  if (!NMEAChecksum(src))
    return 0;

  return ExtractParameters(src, dst, arr, arrsz);
}


bool NMEAParser::ParseNMEAString_Internal(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[MAX_NMEA_LEN];
  const TCHAR *params[MAX_NMEA_PARAMS];
  size_t n_params;

  n_params = ValidateAndExtract(String, ctemp, MAX_NMEA_LEN, params, MAX_NMEA_PARAMS);
  if (n_params < 1 || params[0][0] != '$')
    return false;
  if (EnableLogNMEA)
    LogNMEA(String);

  if(params[0][1] == 'P')
    {
      //Proprietary String

      if(_tcscmp(params[0] + 1,TEXT("PTAS1"))==0)
        {
          return PTAS1(&String[7], params + 1, n_params, GPS_INFO);
        }

      // FLARM sentences
      if(_tcscmp(params[0] + 1,TEXT("PFLAA"))==0)
        {
          return PFLAA(&String[7], params + 1, n_params, GPS_INFO);
        }

      if(_tcscmp(params[0] + 1,TEXT("PFLAU"))==0)
        {
          return PFLAU(&String[7], params + 1, n_params, GPS_INFO);
        }

      if(_tcscmp(params[0] + 1,TEXT("PGRMZ"))==0)
	{
	  return RMZ(&String[7], params + 1, n_params, GPS_INFO);
	}
      return false;
    }

  if(_tcscmp(params[0] + 3,TEXT("GSA"))==0)
    {
      return GSA(&String[7], params + 1, n_params, GPS_INFO);
    }
  if(_tcscmp(params[0] + 3,TEXT("GLL"))==0)
    {
      //    return GLL(&String[7], params + 1, n_params, GPS_INFO);
      return false;
    }
  if(_tcscmp(params[0] + 3,TEXT("RMB"))==0)
    {
      //return RMB(&String[7], params + 1, n_params, GPS_INFO);
          return false;
      }
  if(_tcscmp(params[0] + 3,TEXT("RMC"))==0)
    {
      return RMC(&String[7], params + 1, n_params, GPS_INFO);
    }
  if(_tcscmp(params[0] + 3,TEXT("GGA"))==0)
    {
      return GGA(&String[7], params + 1, n_params, GPS_INFO);
    }

  return false;
}

void NMEAParser::ExtractParameter(const TCHAR *Source,
				  TCHAR *Destination,
				  int DesiredFieldNumber)
{
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength = _tcslen(Source);
  TCHAR *sptr = (TCHAR*)Source;
  const TCHAR *eptr = Source+StringLength;

  if (!Destination) return;

  while( (CurrentFieldNumber < DesiredFieldNumber) && (sptr<eptr) )
    {
      if (*sptr == ',' || *sptr == '*' )
        {
          CurrentFieldNumber++;
        }
      ++sptr;
    }

  Destination[0] = '\0'; // set to blank in case it's not found..

  if ( CurrentFieldNumber == DesiredFieldNumber )
    {
      while( (sptr < eptr)    &&
             (*sptr != ',') &&
             (*sptr != '*') &&
             (*sptr != '\0') )
        {
          Destination[dest_index] = *sptr;
          ++sptr; ++dest_index;
        }
      Destination[dest_index] = '\0';
    }
}


double EastOrWest(double in, TCHAR EoW)
{
  if(EoW == 'W')
    return -in;
  else
    return in;
}

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

int NAVWarn(TCHAR c)
{
  if(c=='A')
    return false;
  else
    return true;
}

double NMEAParser::ParseAltitude(const TCHAR *value, const TCHAR *format)
{
  double alt = StrToDouble(value, NULL);

  if (format[0] == _T('f') || format[0] == _T('F'))
    alt /= TOFEET;

  return alt;
}

double MixedFormatToDegrees(double mixed)
{
  double mins, degrees;

  degrees = (int) (mixed/100);
  mins = (mixed - degrees*100)/60;

  return degrees+mins;
}

double NMEAParser::TimeModify(double FixTime, NMEA_INFO* GPS_INFO)
{
  double hours, mins,secs;

  hours = FixTime / 10000;
  GPS_INFO->Hour = (int)hours;

  mins = FixTime / 100;
  mins = mins - (GPS_INFO->Hour*100);
  GPS_INFO->Minute = (int)mins;

  secs = FixTime - (GPS_INFO->Hour*10000) - (GPS_INFO->Minute*100);
  GPS_INFO->Second = (int)secs;

  FixTime = secs + (GPS_INFO->Minute*60) + (GPS_INFO->Hour*3600);

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


bool NMEAParser::TimeHasAdvanced(double ThisTime, NMEA_INFO *GPS_INFO) {
  if(ThisTime< LastTime) {
    LastTime = ThisTime;
    StartDay = -1; // reset search for the first day
    return false;
  } else {
    GPS_INFO->Time = ThisTime;
    LastTime = ThisTime;
    return true;
  }
}

bool NMEAParser::GSA(const TCHAR *String,
                     const TCHAR **params, size_t nparams, NMEA_INFO *GPS_INFO)
{
  int iSatelliteCount =0;

  if (ReplayLogger::IsEnabled()) {
    return true;
  }

  // satellites are in items 4-15 of GSA string (4-15 is 1-indexed)
  // but 1st item in string is not passed, so start at item 3
  for (int i = 0; i < MAXSATELLITES; i++)
  {
    if (3+i < (int) nparams) {
      GPS_INFO->SatelliteIDs[i] = (int)(StrToDouble(params[2+i], NULL)); // 2 because params is 0-index
      if (GPS_INFO->SatelliteIDs[i] > 0)
	iSatelliteCount ++;
    }
  }


  GSAAvailable = true;
  return true;

}

bool NMEAParser::GLL(const TCHAR *String,
                     const TCHAR **params, size_t nparams, NMEA_INFO *GPS_INFO)
{
  gpsValid = !NAVWarn(params[5][0]);

  if (!activeGPS)
    return true;

  if (ReplayLogger::IsEnabled()) {
    // block actual GPS signal
    InterfaceTimeoutReset();
    return true;
  }

  GPS_INFO->NAVWarning = !gpsValid;

  ////

  double ThisTime = TimeModify(StrToDouble(params[4],NULL), GPS_INFO);
  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return false;

  double tmplat;
  double tmplon;

  tmplat = MixedFormatToDegrees(StrToDouble(params[0], NULL));
  tmplat = NorthOrSouth(tmplat, params[1][0]);

  tmplon = MixedFormatToDegrees(StrToDouble(params[2], NULL));
  tmplon = EastOrWest(tmplon,params[3][0]);

  if (!((tmplat == 0.0) && (tmplon == 0.0))) {
    GPS_INFO->Latitude = tmplat;
    GPS_INFO->Longitude = tmplon;
  } else {

  }
  return true;
}


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

  GPS_INFO->CrossTrackError = NAUTICALMILESTOMETRES * StrToDouble(params[1], NULL);
  GPS_INFO->CrossTrackError = LeftOrRight(GPS_INFO->CrossTrackError,params[2][0]);

  _tcscpy(ctemp, params[4]);
  ctemp[WAY_POINT_ID_SIZE] = '\0';
  _tcscpy(GPS_INFO->WaypointID,ctemp);

  GPS_INFO->WaypointDistance = NAUTICALMILESTOMETRES * StrToDouble(params[9], NULL);
  GPS_INFO->WaypointBearing = StrToDouble(params[10], NULL);
  GPS_INFO->WaypointSpeed = KNOTSTOMETRESSECONDS * StrToDouble(params[11], NULL);
  */

  return true;
}


bool SetSystemTimeFromGPS = false;

bool NMEAParser::RMC(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{
  TCHAR *Stop;

  gpsValid = !NAVWarn(params[1][0]);

  GPSCONNECT = true;

  if (!activeGPS)
    return true;

  double speed = StrToDouble(params[6], NULL);

  if (speed>2.0) {
    GPS_INFO->MovementDetected = true;
    if (ReplayLogger::IsEnabled()) {
      // stop logger replay if aircraft is actually moving.
      ReplayLogger::Stop();
    }
  } else {
    GPS_INFO->MovementDetected = false;
    if (ReplayLogger::IsEnabled()) {
      // block actual GPS signal if not moving and a log is being replayed
      return true;
    }
  }

  GPS_INFO->NAVWarning = !gpsValid;

  // say we are updated every time we get this,
  // so infoboxes get refreshed if GPS connected
  TriggerGPSUpdate();

  // JMW get date info first so TimeModify is accurate
  TCHAR date_buffer[9];
  _tcsncpy(date_buffer, params[8], sizeof(date_buffer) - 1);
  date_buffer[sizeof(date_buffer) - 1] = 0;

  GPS_INFO->Year = _tcstol(&date_buffer[4], &Stop, 10) + 2000;
  date_buffer[4] = '\0';
  GPS_INFO->Month = _tcstol(&date_buffer[2], &Stop, 10);
  date_buffer[2] = '\0';
  GPS_INFO->Day = _tcstol(&date_buffer[0], &Stop, 10);

  double ThisTime = TimeModify(StrToDouble(params[0],NULL), GPS_INFO);
  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return false;

  double tmplat;
  double tmplon;

  tmplat = MixedFormatToDegrees(StrToDouble(params[2], NULL));
  tmplat = NorthOrSouth(tmplat, params[3][0]);

  tmplon = MixedFormatToDegrees(StrToDouble(params[4], NULL));
  tmplon = EastOrWest(tmplon,params[5][0]);

  if (!((tmplat == 0.0) && (tmplon == 0.0))) {
    GPS_INFO->Latitude = tmplat;
    GPS_INFO->Longitude = tmplon;
  }

  GPS_INFO->Speed = KNOTSTOMETRESSECONDS * speed;

  if (GPS_INFO->Speed>1.0) {
    // JMW don't update bearing unless we're moving
    GPS_INFO->TrackBearing = AngleLimit360(StrToDouble(params[7], NULL));
  }

  // Altair doesn't have a battery-backed up realtime clock,
  // so as soon as we get a fix for the first time, set the
  // system clock to the GPS time.
  static bool sysTimeInitialised = false;

  if (!GPS_INFO->NAVWarning && (gpsValid)) {
#if defined(GNAV) && (!defined(WINDOWSPC) || (WINDOWSPC==0))
    SetSystemTimeFromGPS = true;
#endif
    if (SetSystemTimeFromGPS) {
      if (!sysTimeInitialised) {

        SYSTEMTIME sysTime;
        ::GetSystemTime(&sysTime);
        int hours = (int)GPS_INFO->Hour;
        int mins = (int)GPS_INFO->Minute;
        int secs = (int)GPS_INFO->Second;
        sysTime.wYear = (unsigned short)GPS_INFO->Year;
        sysTime.wMonth = (unsigned short)GPS_INFO->Month;
        sysTime.wDay = (unsigned short)GPS_INFO->Day;
        sysTime.wHour = (unsigned short)hours;
        sysTime.wMinute = (unsigned short)mins;
        sysTime.wSecond = (unsigned short)secs;
        sysTime.wMilliseconds = 0;
        sysTimeInitialised = (::SetSystemTime(&sysTime)==true);

#if defined(GNAV) && (!defined(WINDOWSPC) || (WINDOWSPC==0))
        TIME_ZONE_INFORMATION tzi;
        tzi.Bias = -UTCOffset/60;
        _tcscpy(tzi.StandardName,TEXT("Altair"));
        tzi.StandardDate.wMonth= 0; // disable daylight savings
        tzi.StandardBias = 0;
        _tcscpy(tzi.DaylightName,TEXT("Altair"));
        tzi.DaylightDate.wMonth= 0; // disable daylight savings
        tzi.DaylightBias = 0;

        SetTimeZoneInformation(&tzi);
#endif
        sysTimeInitialised =true;

      }
    }
  }

  if (!ReplayLogger::IsEnabled()) {
    if(RMZAvailable)
      {
	// JMW changed from Altitude to BaroAltitude
	GPS_INFO->BaroAltitudeAvailable = true;
	GPS_INFO->BaroAltitude = RMZAltitude;
      }
    else if(RMAAvailable)
      {
	// JMW changed from Altitude to BaroAltitude
	GPS_INFO->BaroAltitudeAvailable = true;
	GPS_INFO->BaroAltitude = RMAAltitude;
      }
  }
  if (!GGAAvailable) {
    // update SatInUse, some GPS receiver dont emmit GGA sentance
    if (!gpsValid) {
      GPS_INFO->SatellitesUsed = 0;
    } else {
      GPS_INFO->SatellitesUsed = -1;
    }
  }

  return true;
}

bool NMEAParser::GGA(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{

  if (ReplayLogger::IsEnabled()) {
    return true;
  }

  GGAAvailable = true;

  nSatellites = (int)(min(16.0, StrToDouble(params[6], NULL)));
  if (nSatellites==0) {
    gpsValid = false;
  }

  if (!activeGPS)
    return true;

  GPS_INFO->SatellitesUsed = (int)(min(16,StrToDouble(params[6], NULL)));

  double ThisTime = TimeModify(StrToDouble(params[0],NULL), GPS_INFO);
  if (!TimeHasAdvanced(ThisTime, GPS_INFO))
    return false;

  double tmplat;
  double tmplon;

  tmplat = MixedFormatToDegrees(StrToDouble(params[1], NULL));
  tmplat = NorthOrSouth(tmplat, params[2][0]);

  tmplon = MixedFormatToDegrees(StrToDouble(params[3], NULL));
  tmplon = EastOrWest(tmplon,params[4][0]);

  if (!((tmplat == 0.0) && (tmplon == 0.0))) {
    GPS_INFO->Latitude = tmplat;
    GPS_INFO->Longitude = tmplon;
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
  GPS_INFO->Altitude = ParseAltitude(params[8], params[9]);
#ifdef FIVV
  GPS_INFO->Altitude += GPSAltitudeOffset;
#endif

  //
  double GeoidSeparation;
  if (_tcslen(params[10])>0) {
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
      GeoidSeparation = LookupGeoidSeparation(GPS_INFO->Latitude,
					      GPS_INFO->Longitude);
      GPS_INFO->Altitude -= GeoidSeparation;
    }
  }

  return true;
}


bool NMEAParser::RMZ(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;

  RMZAltitude = ParseAltitude(params[0], params[1]);
  //JMW?  RMZAltitude = AltitudeToQNHAltitude(RMZAltitude);
  RMZAvailable = true;

  if (!devHasBaroSource()) {
    // JMW no in-built baro sources, so use this generic one
    if (!ReplayLogger::IsEnabled()) {
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMZAltitude;
    }
  }

  return false;
}


bool NMEAParser::RMA(const TCHAR *String, const TCHAR **params, size_t nparams,
                     NMEA_INFO *GPS_INFO)
{
  (void)GPS_INFO;

  RMAAltitude = ParseAltitude(params[0], params[1]);
  //JMW?  RMAAltitude = AltitudeToQNHAltitude(RMAAltitude);
  RMAAvailable = true;
  GPS_INFO->BaroAltitudeAvailable = true;

  if (!devHasBaroSource()) {
    if (!ReplayLogger::IsEnabled()) {
      // JMW no in-built baro sources, so use this generic one
      GPS_INFO->BaroAltitudeAvailable = true;
      GPS_INFO->BaroAltitude = RMAAltitude;
    }
  }

  return false;
}


bool NMEAParser::NMEAChecksum(const TCHAR *String)
{
  unsigned char CalcCheckSum = 0;
  unsigned char ReadCheckSum;
  int End;
  int i;
  TCHAR c1,c2;
  unsigned char v1 = 0,v2 = 0;
  TCHAR *pEnd;

  pEnd = _tcschr((TCHAR*)String,'*');
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

  End =(int)( pEnd - String);

  for(i=1;i<End;i++)
    {
      CalcCheckSum = (unsigned char)(CalcCheckSum ^ String[i]);
    }

  if(CalcCheckSum == ReadCheckSum)
    return true;
  else
    return false;
}

//////



bool NMEAParser::PTAS1(const TCHAR *String,
                       const TCHAR **params, size_t nparams,
                       NMEA_INFO *GPS_INFO)
{
  double wnet,baralt,vtas;

  wnet = (StrToDouble(params[0],NULL)-200)/(10*TOKNOTS);
  baralt = max(0.0, (StrToDouble(params[2],NULL)-2000)/TOFEET);
  vtas = StrToDouble(params[3],NULL)/TOKNOTS;

  GPS_INFO->AirspeedAvailable = true;
  GPS_INFO->TrueAirspeed = vtas;
  GPS_INFO->VarioAvailable = true;
  GPS_INFO->Vario = wnet;
  GPS_INFO->BaroAltitudeAvailable = true;
  GPS_INFO->BaroAltitude = AltitudeToQNHAltitude(baralt);
  GPS_INFO->IndicatedAirspeed = vtas/AirDensityRatio(baralt);

  TriggerVarioUpdate();

  return false;
}


double AccelerometerZero=100.0;

////////////// FLARM

void FLARM_RefreshSlots(NMEA_INFO *GPS_INFO) {
  int i;
  bool present = false;
  if (GPS_INFO->FLARM_Available) {

    for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
      if (GPS_INFO->FLARM_Traffic[i].ID>0) {
	if ((GPS_INFO->Time> GPS_INFO->FLARM_Traffic[i].Time_Fix+2)
	    || (GPS_INFO->Time< GPS_INFO->FLARM_Traffic[i].Time_Fix)) {
	  // clear this slot if it is too old (2 seconds), or if
	  // time has gone backwards (due to replay)
	  GPS_INFO->FLARM_Traffic[i].ID= 0;
	  GPS_INFO->FLARM_Traffic[i].Name[0] = 0;
	} else {
          if (GPS_INFO->FLARM_Traffic[i].AlarmLevel>0) {
            GaugeFLARM::Suppress = false;
          }
	  present = true;
	}
      }
    }
  }
  GaugeFLARM::TrafficPresent(present);
}


#include "InputEvents.h"

double FLARM_NorthingToLatitude = 0.0;
double FLARM_EastingToLongitude = 0.0;


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

  double dlat;
  DistanceBearing(GPS_INFO->Latitude, GPS_INFO->Longitude,
                  GPS_INFO->Latitude+delta_lat, GPS_INFO->Longitude,
                  &dlat, NULL);
  double dlon;
  DistanceBearing(GPS_INFO->Latitude, GPS_INFO->Longitude,
                  GPS_INFO->Latitude, GPS_INFO->Longitude+delta_lon,
                  &dlon, NULL);

  if ((fabs(dlat)>0.0)&&(fabs(dlon)>0.0)) {
    FLARM_NorthingToLatitude = delta_lat / dlat;
    FLARM_EastingToLongitude = delta_lon / dlon;
  } else {
    FLARM_NorthingToLatitude=0.0;
    FLARM_EastingToLongitude=0.0;
  }

  _stscanf(String,
	  TEXT("%hu,%hu,%hu,%hu"),
	  &GPS_INFO->FLARM_RX, // number of received FLARM devices
	  &GPS_INFO->FLARM_TX, // Transmit status
	  &GPS_INFO->FLARM_GPS, // GPS status
	  &GPS_INFO->FLARM_AlarmLevel); // Alarm level of FLARM (0-3)

  // process flarm updates

  if ((GPS_INFO->FLARM_RX) && (old_flarm_rx==0)) {
    // traffic has appeared..
    InputEvents::processGlideComputer(GCE_FLARM_TRAFFIC);
  }
  if (GPS_INFO->FLARM_RX > old_flarm_rx) {
    // re-set suppression of gauge, as new traffic has arrived
    //    GaugeFLARM::Suppress = false;
  }
  if ((GPS_INFO->FLARM_RX==0) && (old_flarm_rx)) {
    // traffic has disappeared..
    InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);
  }
  // TODO feature: add another event for new traffic.

  old_flarm_rx = GPS_INFO->FLARM_RX;

  return false;
}


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
      GaugeFLARM::Suppress = false;
      return i;
    }
  }

  // still not found and no empty slots left, buffer is full
  return -1;
}



bool NMEAParser::PFLAA(const TCHAR *String,
                       const TCHAR **params, size_t nparams,
                       NMEA_INFO *GPS_INFO)
{
  int flarm_slot = 0;

  isFlarm = true;

  // 5 id, 6 digit hex
  long ID;
  _stscanf(params[5],TEXT("%lx"), &ID);
//  unsigned long uID = ID;

  flarm_slot = FLARM_FindSlot(GPS_INFO, ID);
  if (flarm_slot<0) {
    // no more slots available,
    return false;
  }

  // set time of fix to current time
  GPS_INFO->FLARM_Traffic[flarm_slot].Time_Fix = GPS_INFO->Time;

  _stscanf(String,
	  TEXT("%hu,%lf,%lf,%lf,%hu,%lx,%lf,%lf,%lf,%lf,%hu"),
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
  GPS_INFO->FLARM_Traffic[flarm_slot].Latitude =
    GPS_INFO->FLARM_Traffic[flarm_slot].RelativeNorth
    *FLARM_NorthingToLatitude + GPS_INFO->Latitude;
  // 2 relativeeast, meters
  GPS_INFO->FLARM_Traffic[flarm_slot].Longitude =
    GPS_INFO->FLARM_Traffic[flarm_slot].RelativeEast
    *FLARM_EastingToLongitude + GPS_INFO->Longitude;
  // alt
  GPS_INFO->FLARM_Traffic[flarm_slot].Altitude =
    GPS_INFO->FLARM_Traffic[flarm_slot].RelativeAltitude +
    GPS_INFO->Altitude;


#ifdef FLARM_AVERAGE
  GPS_INFO->FLARM_Traffic[flarm_slot].Average30s = flarmCalculations.Average30s(
	  GPS_INFO->FLARM_Traffic[flarm_slot].ID,
	  GPS_INFO->Time,
	  GPS_INFO->FLARM_Traffic[flarm_slot].Altitude);
#endif

  TCHAR *name = GPS_INFO->FLARM_Traffic[flarm_slot].Name;
  if (!_tcslen(name)) {
    // need to lookup name for this target
    const TCHAR *fname = LookupFLARMDetails(GPS_INFO->FLARM_Traffic[flarm_slot].ID);
    if (fname) {
      _tcscpy(name,fname);
    } else {
      name[0]=0;
    }
  }

  if ((GPS_INFO->FLARM_Traffic[flarm_slot].ID == TeamFlarmIdTarget)
      && ValidWayPoint(TeamCodeRefWaypoint)) {
    double bearing;
    double distance;

    TeammateLatitude = GPS_INFO->FLARM_Traffic[flarm_slot].Latitude;
    TeammateLongitude = GPS_INFO->FLARM_Traffic[flarm_slot].Longitude;
    DistanceBearing(WayPointList[TeamCodeRefWaypoint].Latitude,
		    WayPointList[TeamCodeRefWaypoint].Longitude,
		    GPS_INFO->FLARM_Traffic[flarm_slot].Latitude,
		    GPS_INFO->FLARM_Traffic[flarm_slot].Longitude,
		    &distance,
		    &bearing);

    GetTeamCode(TeammateCode, bearing, distance);
    TeammateCodeValid = true;
  }

  return false;
}


//////

void NMEAParser::TestRoutine(NMEA_INFO *GPS_INFO) {
#ifndef NDEBUG
#ifndef GNAV
  static int i=90;
  static TCHAR t1[] = TEXT("1,1,1,1");
  static TCHAR t2[] = TEXT("1,300,500,220,2,DD927B,0,-4.5,30,-1.4,1");
  static TCHAR t3[] = TEXT("0,0,1200,50,2,DD9146,270,-4.5,30,-1.4,1");
  //  static TCHAR b50[] = TEXT("0,.1,.0,0,0,1.06,0,-222");
  //  static TCHAR t4[] = TEXT("-3,500,1024,50");

  //  nmeaParser1.ParseNMEAString_Internal(TEXT("$PTAS1,201,200,02583,000*2A"), GPS_INFO);
  //  nmeaParser1.ParseNMEAString_Internal(TEXT("$GPRMC,082430.00,A,3744.09096,S,14426.16069,E,0.520294.90,301207,,,A*77"), GPS_INFO);
  //  nmeaParser1.ParseNMEAString_Internal(TEXT("$GPGGA,082430.00,3744.09096,S,1426.16069,E,1,08,1.37,157.6,M,-4.9,M,,*5B"), GPS_INFO);

  QNH=1013.25;
  double h;
  double altraw= 5.0;
  h = AltitudeToQNHAltitude(altraw);
  QNH = FindQNH(altraw, 50.0);
  h = AltitudeToQNHAltitude(altraw);

  ////

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


///

bool EnableLogNMEA = false;
HANDLE nmeaLogFile = INVALID_HANDLE_VALUE;

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
    nmeaLogFile = CreateFile(TEXT("\\SD Card\\xcsoar-nmea.log"),
			     GENERIC_WRITE, FILE_SHARE_WRITE,
			     NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  }

  WriteFile(nmeaLogFile, text, _tcslen(text)*sizeof(TCHAR), &dwBytesRead,
	    (OVERLAPPED *)NULL);
}


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
