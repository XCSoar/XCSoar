/*
  $Id$

Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"

#include "externs.h"
#include "utils.h"
#include "externs.h"
#include "VarioSound.h"
#include "Logger.h"
#include "GaugeFLARM.h"
#include "parser.h"

extern bool EnableCalibration;

static double EastOrWest(double in, TCHAR EoW);
static double NorthOrSouth(double in, TCHAR NoS);
static double LeftOrRight(double in, TCHAR LoR);
static double AltitudeModify(double Altitude, TCHAR Format);
static double MixedFormatToDegrees(double mixed);
static double TimeModify(double FixTime);
static int NAVWarn(TCHAR c);

static BOOL RMZAvailable = FALSE;
static double RMZAltitude = 0;
static BOOL RMAAvailable = FALSE;
static double RMAAltitude = 0;

BOOL NMEAParser::GpsUpdated = false;
BOOL NMEAParser::VarioUpdated = false;

NMEAParser nmeaParser1;
NMEAParser nmeaParser2;


NMEAParser::NMEAParser() {
  gpsValid = false;
  hasVega = false;
  nSatellites = 0;

  activeGPS = false;
}

void NMEAParser::Reset(void) {
  GpsUpdated = TRUE;
  SetEvent(dataTriggerEvent);
  PulseEvent(varioTriggerEvent);
  VarioUpdated = TRUE;
  nmeaParser1.gpsValid = false;
  nmeaParser2.gpsValid = false;
  nmeaParser1.activeGPS = true;
  nmeaParser2.activeGPS = true;
  nmeaParser1.hasVega = false;
  nmeaParser2.hasVega = false;
}


int NMEAParser::FindVegaPort(void) {
  if (nmeaParser1.hasVega)
    return 0;
  if (nmeaParser2.hasVega)
    return 1;
  return -1;
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


BOOL NMEAParser::ParseNMEAString(int device,
				 TCHAR *String, NMEA_INFO *GPS_INFO)
{
  switch (device) {
  case 0:
    return nmeaParser1.ParseNMEAString_Internal(String, GPS_INFO);
  case 1:
    return nmeaParser2.ParseNMEAString_Internal(String, GPS_INFO);
  };
  return FALSE;
}


BOOL NMEAParser::ParseNMEAString_Internal(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR SentanceString[6] = TEXT("");
  int i;

  if(_tcslen(String)<=6)
    {
      // string is too short
      return FALSE;
    }
  if(_tcslen(String)>90)
    {
      // string is too long
      return FALSE;
    }

  if(_tcslen(String)>90)

  if(String[0] != '$')
    {
      return FALSE;
    }
  if(!NMEAChecksum(String))
    {
      return FALSE;
    }
  if (EnableLogNMEA)
    LogNMEA(String);

  if(String[1] == 'P')
    {
      //Proprietary String
      // JMW added Vega variometer

      for(i=0;i<5;i++)
        {
          SentanceString[i] = String[i+1];
        }
      SentanceString[5] = '\0';

      if(_tcscmp(SentanceString,TEXT("PBJVA"))==0)
        {
          return PBJVA(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PJV01"))==0)
        {
          return PJV01(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PBB50"))==0)
        {
          return PBB50(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PBJVH"))==0)
        {
          return PBJVH(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PDSWC"))==0)
        {
          return PDSWC(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PDAAV"))==0)
        {
          return PDAAV(&String[7], GPS_INFO);
        }

      if(_tcscmp(SentanceString,TEXT("PDVSC"))==0)
        {
          return PDVSC(&String[7], GPS_INFO);
        }

      if(_tcscmp(SentanceString,TEXT("PDVDV"))==0)
        {
          return PDVDV(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PDVDS"))==0)
        {
          return PDVDS(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PDVVT"))==0)
        {
          return PDVVT(&String[7], GPS_INFO);
        }
      if(_tcscmp(SentanceString,TEXT("PDVSD"))==0)
	{
	  TCHAR cptext[80];
	  wsprintf(cptext,TEXT("%s"), &String[7]);
	  // TODO - JMW (from Scott)
	  // 	Either use something like
	  // 		DoStatusMessage(TEXT("Vario Message"), cptext);
	  // 		(then you can assign time and sound to Vario Message)
	  // 	or	Message::AddMessage
	  DoStatusMessage(cptext);
	  return FALSE;
	}

      if(_tcscmp(SentanceString,TEXT("PFLAA"))==0)
        {
          return PFLAA(&String[7], GPS_INFO);
        }

      if(_tcscmp(SentanceString,TEXT("PFLAU"))==0)
        {
          return PFLAU(&String[7], GPS_INFO);
        }

      return FALSE;
    }

  // basic GPS sentences
  for(i=0;i<3;i++)
    {
      SentanceString[i] = String[i+3];
    }

  SentanceString[3] = '\0';

  if(_tcscmp(SentanceString,TEXT("GLL"))==0)
    {
      //    return GLL(&String[7], GPS_INFO);
      return FALSE;
    }
  if(_tcscmp(SentanceString,TEXT("RMB"))==0)
    {
      //return RMB(&String[7], GPS_INFO);
      return FALSE;
    }
  if(_tcscmp(SentanceString,TEXT("RMC"))==0)
    {
      return RMC(&String[7], GPS_INFO);
    }
  if(_tcscmp(SentanceString,TEXT("GGA"))==0)
    {
      return GGA(&String[7], GPS_INFO);
    }
  if(_tcscmp(SentanceString,TEXT("RMZ"))==0)
    {
      return RMZ(&String[7], GPS_INFO);
    }
  if(_tcscmp(SentanceString,TEXT("WP0"))==0)
    {
      return WP0(&String[7], GPS_INFO);
    }
  if(_tcscmp(SentanceString,TEXT("WP1"))==0)
    {
      return WP1(&String[7], GPS_INFO);
    }
 if(_tcscmp(SentanceString,TEXT("WP2"))==0)
    {
      return WP2(&String[7], GPS_INFO);
    }
  return FALSE;
}

void NMEAParser::ExtractParameter(TCHAR *Source,
				  TCHAR *Destination,
				  int DesiredFieldNumber)
{
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength = _tcslen(Source);
  TCHAR *sptr = Source;
  TCHAR *eptr = Source+StringLength;

  while( (CurrentFieldNumber < DesiredFieldNumber) && (sptr<eptr) )
    {
      if (*sptr == ',' || *sptr == '*' )
        {
          CurrentFieldNumber++;
        }
      ++sptr;
    }

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

double LeftOrRight(double in, TCHAR LoR)
{
  if(LoR == 'L')
    return -in;
  else
    return in;
}

int NAVWarn(TCHAR c)
{
  if(c=='A')
    return FALSE;
  else
    return TRUE;
}

double AltitudeModify(double Altitude, TCHAR Format)
{
  if(Format == 'M')
    return Altitude;
  else if ((Format == 'f') || (Format == 'F'))
    return Altitude / TOFEET;
  else
    return Altitude;
}

double MixedFormatToDegrees(double mixed)
{
  double mins, degrees;

  degrees = (int) (mixed/100);
  mins = mixed - (degrees*100);
  mins = mins/60;

  return degrees+mins;
}

double TimeModify(double FixTime)
{
  double hours, mins,secs;

  hours = FixTime / 10000;
  hours = (double)(int)hours;
  mins = FixTime / 100;
  mins = mins - (hours*100);
  mins = (double)(int)mins;
  secs = FixTime - (hours*10000) - (mins*100);

  FixTime = secs + (mins*60) + (hours*3600);

  return FixTime;
}


BOOL NMEAParser::GLL(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  double ThisTime;
  static double LastTime = 0;

  ExtractParameter(String,ctemp,5);
  gpsValid = !NAVWarn(ctemp[0]);

  if (activeGPS) {

    if (ReplayLogger::IsEnabled()) {
      // block actual GPS signal
      return TRUE;
    }

    GPS_INFO->NAVWarning = !gpsValid;

    ////

    ExtractParameter(String,ctemp,4);
    ThisTime = StrToDouble(ctemp,NULL);
    ThisTime = TimeModify(ThisTime);

    if(ThisTime<=LastTime)
      {
	LastTime = ThisTime;
	return FALSE;
      }

    LastTime = ThisTime;
    GPS_INFO->Time = ThisTime;

    double tmplat;
    double tmplon;

    ExtractParameter(String,ctemp,0);
    tmplat = MixedFormatToDegrees(StrToDouble(ctemp, NULL));
    ExtractParameter(String,ctemp,1);
    tmplat = NorthOrSouth(tmplat, ctemp[0]);

    ExtractParameter(String,ctemp,2);
    tmplon = MixedFormatToDegrees(StrToDouble(ctemp, NULL));

    ExtractParameter(String,ctemp,3);
    tmplon = EastOrWest(tmplon,ctemp[0]);

    if (!((tmplat == 0.0) && (tmplon == 0.0))) {
      GPS_INFO->Latitude = tmplat;
      GPS_INFO->Longitude = tmplon;
    } else {

    }
  }
  return TRUE;
}


BOOL NMEAParser::RMB(TCHAR *String, NMEA_INFO *GPS_INFO)
{

  /* we calculate all this stuff now
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  GPS_INFO->NAVWarning = NAVWarn(ctemp[0]);

  ExtractParameter(String,ctemp,1);
  GPS_INFO->CrossTrackError = NAUTICALMILESTOMETRES * StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,2);
  GPS_INFO->CrossTrackError = LeftOrRight(GPS_INFO->CrossTrackError,ctemp[0]);

  ExtractParameter(String,ctemp,4);
  ctemp[WAY_POINT_ID_SIZE] = '\0';

  _tcscpy(GPS_INFO->WaypointID,ctemp);

  ExtractParameter(String,ctemp,9);
  GPS_INFO->WaypointDistance = NAUTICALMILESTOMETRES * StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,10);
  GPS_INFO->WaypointBearing = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,11);
  GPS_INFO->WaypointSpeed = KNOTSTOMETRESSECONDS * StrToDouble(ctemp, NULL);
  */

  return TRUE;
}

BOOL NMEAParser::RMC(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  TCHAR *Stop;
  double ThisTime;
  static double LastTime = 0;

  ExtractParameter(String,ctemp,1);
  gpsValid = !NAVWarn(ctemp[0]);

  GPSCONNECT = TRUE;

  if (activeGPS) {

    ExtractParameter(String,ctemp,6);
    double speed = StrToDouble(ctemp, NULL);

    if (ReplayLogger::IsEnabled()) {
      if (speed>2.0) {
	// stop logger replay if aircraft is actually moving.
	ReplayLogger::Stop();
      } else {
	// block actual GPS signal
      }
      return TRUE;
    }

    GPS_INFO->NAVWarning = !gpsValid;

    ExtractParameter(String,ctemp,0);
    ThisTime = StrToDouble(ctemp, NULL);
    ThisTime = TimeModify(ThisTime);

    // say we are updated every time we get this,
    // so infoboxes get refreshed if GPS connected
    GpsUpdated = TRUE;
    SetEvent(dataTriggerEvent);

    if(ThisTime<=LastTime)
      {
	LastTime = ThisTime;
	return FALSE;
      }

    ////////

    double tmplat;
    double tmplon;

    ExtractParameter(String,ctemp,2);
    tmplat = MixedFormatToDegrees(StrToDouble(ctemp, NULL));
    ExtractParameter(String,ctemp,3);
    tmplat = NorthOrSouth(tmplat, ctemp[0]);

    ExtractParameter(String,ctemp,4);
    tmplon = MixedFormatToDegrees(StrToDouble(ctemp, NULL));
    ExtractParameter(String,ctemp,5);
    tmplon = EastOrWest(tmplon,ctemp[0]);

    if (!((tmplat == 0.0) && (tmplon == 0.0))) {
      GPS_INFO->Latitude = tmplat;
      GPS_INFO->Longitude = tmplon;
    } else {
    }

    ExtractParameter(String,ctemp,6);
    GPS_INFO->Speed = KNOTSTOMETRESSECONDS * speed;

    ExtractParameter(String,ctemp,7);

    if (GPS_INFO->Speed>1.0) {
      // JMW don't update bearing unless we're moving
      GPS_INFO->TrackBearing = StrToDouble(ctemp, NULL);
    }

    ExtractParameter(String,ctemp,8);
    GPS_INFO->Year = _tcstol(&ctemp[4], &Stop, 10) + 2000;
    ctemp[4] = '\0';
    GPS_INFO->Month = _tcstol(&ctemp[2], &Stop, 10);
    ctemp[2] = '\0';
    GPS_INFO->Day = _tcstol(&ctemp[0], &Stop, 10);

    if(GPS_INFO->Day > 1)
      {
	GPS_INFO->Time = (( GPS_INFO->Day -1) * 86400) + ThisTime;
      }
    else
      {
	GPS_INFO->Time = ThisTime;
      }

    if(RMZAvailable)
      {
	GPS_INFO->Altitude = RMZAltitude;
      }
    else if(RMAAvailable)
	{
	  GPS_INFO->Altitude = RMAAltitude;
	}

    LastTime = ThisTime;
  }

  return TRUE;
}


BOOL NMEAParser::GGA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  double ThisTime;
  static double LastTime = 0;

  if (ReplayLogger::IsEnabled()) {
    return TRUE;
  }


  ExtractParameter(String,ctemp,6);
  nSatellites = (int)(min(12,StrToDouble(ctemp, NULL)));
  if (nSatellites==0) {
    gpsValid = false;
  }

  if (activeGPS) {
    GPS_INFO->SatellitesUsed = (int)(min(12,StrToDouble(ctemp, NULL)));

    ExtractParameter(String,ctemp,0);
    ThisTime = StrToDouble(ctemp, NULL);
    ThisTime = TimeModify(ThisTime);

    if(ThisTime<=LastTime)
      {
	LastTime = ThisTime;
	return FALSE;
      }

    double tmplat;
    double tmplon;

    ExtractParameter(String,ctemp,1);
    tmplat = MixedFormatToDegrees(StrToDouble(ctemp, NULL));

    ExtractParameter(String,ctemp,2);
    tmplat = NorthOrSouth(tmplat, ctemp[0]);

    ExtractParameter(String,ctemp,3);
    tmplon = MixedFormatToDegrees(StrToDouble(ctemp, NULL));

    ExtractParameter(String,ctemp,4);
    tmplon = EastOrWest(tmplon,ctemp[0]);

    if (!((tmplat == 0.0) && (tmplon == 0.0))) {
	GPS_INFO->Latitude = tmplat;
	GPS_INFO->Longitude = tmplon;
    }

    if(RMZAvailable)
      {
	GPS_INFO->Altitude = RMZAltitude;
      }
    else if(RMAAvailable)
      {
	GPS_INFO->Altitude = RMAAltitude;
      }
    else
      {
	ExtractParameter(String,ctemp,8);
	GPS_INFO->Altitude = StrToDouble(ctemp, NULL);
	ExtractParameter(String,ctemp,9);
	GPS_INFO->Altitude = AltitudeModify(GPS_INFO->Altitude,ctemp[0]);
      }

    if(GPS_INFO->Day > 1)
      {
	GPS_INFO->Time = (( GPS_INFO->Day -1) * 86400) + ThisTime;
      }
    else
      {
	GPS_INFO->Time = ThisTime;
      }
    LastTime = ThisTime;
  }

  return TRUE;
}


BOOL NMEAParser::RMZ(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  RMZAltitude = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  RMZAltitude = AltitudeModify(RMZAltitude,ctemp[0]);
  RMZAvailable = TRUE;
  return FALSE;
}


BOOL NMEAParser::RMA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  RMAAltitude = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  RMAAltitude = AltitudeModify(RMAAltitude,ctemp[0]);
  RMAAvailable = TRUE;
  return FALSE;
}


BOOL NMEAParser::WP0(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  GPS_INFO->BaroAltitudeAvailable = TRUE;
  ExtractParameter(String,ctemp,2);
  GPS_INFO->BaroAltitude = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  GPS_INFO->IndicatedAirspeed = StrToDouble(ctemp, NULL)/TOKPH;
  // JMW TODO check, is this indicated or true airspeed?

  return FALSE;
}

BOOL NMEAParser::WP1(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  return FALSE;
}


BOOL NMEAParser::WP2(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  MACCREADY = StrToDouble(ctemp,NULL);
  return FALSE;
}



BOOL NMEAParser::NMEAChecksum(TCHAR *String)
{
  unsigned char CalcCheckSum = 0;
  unsigned char ReadCheckSum;
  int End;
  int i;
  TCHAR c1,c2;
  unsigned char v1,v2;
  TCHAR *pEnd;

  pEnd = _tcschr(String,'*');
  if(pEnd == NULL)
    return FALSE;

  if(_tcslen(pEnd)<3)
    return FALSE;

  c1 = pEnd[1], c2 = pEnd[2];

  iswdigit('0');

  if(_istdigit(c1))
    v1 = c1 - '0';
  if(_istdigit(c2))
    v2 = c2 - '0';
  if(_istalpha(c1))
    v1 = c1 - 'A' + 10;
  if(_istalpha(c2))
    v2 = c2 - 'A' + 10;

  ReadCheckSum = (v1<<4) + v2;

  End =(int)( pEnd - String);

  for(i=1;i<End;i++)
    {
      CalcCheckSum = CalcCheckSum ^ String[i];
    }

  if(CalcCheckSum == ReadCheckSum)
    return TRUE;
  else
    return FALSE;
}

//////

BOOL NMEAParser::PBB50(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double vtas, vias, wnet;
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  vtas = StrToDouble(ctemp,NULL)/TOKNOTS;

  ExtractParameter(String,ctemp,1);
  wnet = StrToDouble(ctemp,NULL)/TOKNOTS;

  ExtractParameter(String,ctemp,2);
  GPS_INFO->MacReady = StrToDouble(ctemp,NULL)/TOKNOTS;
  MACCREADY = GPS_INFO->MacReady;

  ExtractParameter(String,ctemp,3);
  vias = sqrt(StrToDouble(ctemp,NULL))/TOKNOTS;

  ExtractParameter(String,ctemp,4);
  GPS_INFO->Ballast = StrToDouble(ctemp,NULL)-1.0;
  BALLAST = GPS_INFO->Ballast;
  // JMW TODO: fix this, because for Borgelt it's % of empty weight,
  // for us, it's % of ballast capacity

  // for Borgelt, it's % degradation,
  // for us, it is % of max performance
  ExtractParameter(String,ctemp,5);
  GPS_INFO->Bugs = 1.0/(1.0+StrToDouble(ctemp,NULL));
  BUGS = GPS_INFO->Bugs;

  GPS_INFO->AirspeedAvailable = TRUE;
  GPS_INFO->IndicatedAirspeed = vias;
  GPS_INFO->TrueAirspeed = vtas;
  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->Vario = wnet;

  VarioUpdated = TRUE;
  PulseEvent(varioTriggerEvent);

  return FALSE;
}

double AccelerometerZero=100.0;

BOOL NMEAParser::PBJVA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  int xval, zval;
  TCHAR ctemp[80];
  TCHAR *Stop;

  ExtractParameter(String,ctemp,0);
  if (ctemp[0]=='+') {
    xval = _tcstol(ctemp+1, &Stop, 10);
  } else {
    xval = _tcstol(ctemp, &Stop, 10);
  }
  ExtractParameter(String,ctemp,1);
  if (ctemp[0]=='+') {
    zval = _tcstol(ctemp+1, &Stop, 10);
  } else {
    zval = _tcstol(ctemp, &Stop, 10);
  }

  int mag = isqrt4(xval*xval+zval*zval);
  GPS_INFO->AccelX = xval/AccelerometerZero;
  GPS_INFO->AccelZ = zval/AccelerometerZero;
  GPS_INFO->Gload = mag/AccelerometerZero;
  GPS_INFO->AccelerationAvailable = TRUE;

  return FALSE;
}



BOOL NMEAParser::PBJVH(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double rh, oat;
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  rh = StrToDouble(ctemp,NULL)/100.0;
  ExtractParameter(String,ctemp,1);
  oat = StrToDouble(ctemp,NULL)/100.0;

  oat = oat/10.0-273.15;
  rh = rh/10.0;

  return FALSE;
}


double StaticPressureToAltitude(double ps) {
  double altitude;
  // http://wahiduddin.net/calc/density_altitude.htm

  const double k1=0.190263;
  const double k2=8.417286e-5;
  double h_gps0 = 0;

  double Pa = pow(
                  pow(ps-(QNH-1013.25)*100.0,k1)
                  -(k2*h_gps0)
                  ,(1.0/k1));

  altitude = 44330.8-4946.54*pow(Pa,k1);
  return altitude;

}


double AirDensity(double altitude) {
  double rho = pow((44330.8-altitude)/42266.5,1.0/0.234969);
  return rho;
}


double AirDensityRatio(double altitude) {
  double rho = pow((44330.8-altitude)/42266.5,1.0/0.234969);
  double rho_rat = sqrt(1.225/rho);
  return rho_rat;
}


BOOL NMEAParser::PJV01(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double vias, wnet, vtas;
  TCHAR ctemp[80];
  int pstatic;
  TCHAR *Stop;

  ExtractParameter(String,ctemp,0);
  vias = StrToDouble(ctemp,NULL)/TOKNOTS;

  double kcal = 1.0;
  if (vias<33.0) {
    kcal = (1.0-1.25)/(33.0-15.0)*(vias-15.0)+1.25;
  } else {
    kcal = 1.0;
  }
  vias = vias*kcal;


  vtas = vias*AirDensityRatio(GPS_INFO->BaroAltitude);

  ExtractParameter(String,ctemp,1);
  if (ctemp[0]=='+') {
    wnet = StrToDouble(ctemp+1,NULL);
  } else {
    wnet = StrToDouble(ctemp,NULL);
  }

  ExtractParameter(String,ctemp,2);
  pstatic = _tcstol(ctemp, &Stop, 10);

  // for testing only, this is really static pressure
  GPS_INFO->BaroAltitude = StaticPressureToAltitude(pstatic*10.0);
  GPS_INFO->BaroAltitudeAvailable = TRUE;

  GPS_INFO->AirspeedAvailable = TRUE;
  GPS_INFO->IndicatedAirspeed = vias;
  GPS_INFO->TrueAirspeed = vtas;
  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->Vario = wnet/TOKNOTS;

  VarioUpdated = TRUE;
  PulseEvent(varioTriggerEvent);

  return TRUE;
}

#include "InputEvents.h"


#define INPUT_BIT_FLAP_POS                  0 // 1 flap pos
#define INPUT_BIT_FLAP_ZERO                 1 // 1 flap zero
#define INPUT_BIT_FLAP_NEG                  2 // 1 flap neg
#define INPUT_BIT_SC                        3 // 1 circling
#define INPUT_BIT_GEAR_EXTENDED             5 // 1 gear extended
#define INPUT_BIT_AIRBRAKENOTLOCKED         6 // 1 airbrake extended
#define INPUT_BIT_AUX                       7 // unused?
#define INPUT_BIT_ACK                       8 // 1 ack pressed
#define INPUT_BIT_REP                       9 // 1 rep pressed
#define INPUT_BIT_STALL                     20  // 1 if detected
#define INPUT_BIT_USERSWUP                  23 // 1 if up
#define INPUT_BIT_USERSWMIDDLE              24 // 1 if middle
#define INPUT_BIT_USERSWDOWN                25
#define OUTPUT_BIT_CIRCLING                 0  // 1 if circling


BOOL NMEAParser::PDSWC(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  static long last_switchinputs;
  static long last_switchoutputs;

  unsigned long uswitchinputs, uswitchoutputs;
  swscanf(String,
	  TEXT("%lf,%lx,%lx,%lf"),
	  &MACCREADY,
	  &uswitchinputs,
	  &uswitchoutputs,
	  &GPS_INFO->SupplyBatteryVoltage);

  long switchinputs = uswitchinputs;
  long switchoutputs = uswitchoutputs;

  MACCREADY /= 10;
  GPS_INFO->SupplyBatteryVoltage/= 10;

  GPS_INFO->SwitchState.AirbrakeExtended =
    (switchinputs & (1<<INPUT_BIT_AIRBRAKENOTLOCKED));
  GPS_INFO->SwitchState.FlapPositive =
    (switchinputs & (1<<INPUT_BIT_FLAP_POS));
  GPS_INFO->SwitchState.FlapNeutral =
    (switchinputs & (1<<INPUT_BIT_FLAP_ZERO));
  GPS_INFO->SwitchState.FlapNegative =
    (switchinputs & (1<<INPUT_BIT_FLAP_NEG));
  GPS_INFO->SwitchState.GearExtended =
    (switchinputs & (1<<INPUT_BIT_GEAR_EXTENDED));
  GPS_INFO->SwitchState.Acknowledge =
    (switchinputs & (1<<INPUT_BIT_ACK));
  GPS_INFO->SwitchState.Repeat =
    (switchinputs & (1<<INPUT_BIT_REP));
  GPS_INFO->SwitchState.SpeedCommand =
    (switchinputs & (1<<INPUT_BIT_SC));
  GPS_INFO->SwitchState.UserSwitchUp =
    (switchinputs & (1<<INPUT_BIT_USERSWUP));
  GPS_INFO->SwitchState.UserSwitchMiddle =
    (switchinputs & (1<<INPUT_BIT_USERSWMIDDLE));
  GPS_INFO->SwitchState.UserSwitchDown =
    (switchinputs & (1<<INPUT_BIT_USERSWDOWN));
  GPS_INFO->SwitchState.VarioCircling =
    (switchinputs & (1<<OUTPUT_BIT_CIRCLING));
  GPS_INFO->SwitchState.Stall =
    (switchinputs & (1<<INPUT_BIT_STALL));

  long up_switchinputs;
  long down_switchinputs;
  long up_switchoutputs;
  long down_switchoutputs;

  // detect changes to ON: on now (x) and not on before (!lastx)
  // detect changes to OFF: off now (!x) and on before (lastx)

  down_switchinputs = (switchinputs & (~last_switchinputs));
  up_switchinputs = ((~switchinputs) & (last_switchinputs));
  down_switchoutputs = (switchoutputs & (~last_switchoutputs));
  up_switchoutputs = ((~switchoutputs) & (last_switchoutputs));

  int i;
  long thebit;
  for (i=0; i<32; i++) {
    thebit = 1<<i;
    if ((down_switchinputs & thebit) == thebit) {
      InputEvents::processNmea(i);
    }
    if ((down_switchoutputs & thebit) == thebit) {
      InputEvents::processNmea(i+32);
    }
    if ((up_switchinputs & thebit) == thebit) {
      InputEvents::processNmea(i+64);
    }
    if ((up_switchoutputs & thebit) == thebit) {
      InputEvents::processNmea(i+96);
    }
  }

  last_switchinputs = switchinputs;
  last_switchoutputs = switchoutputs;

  return TRUE;
}


// $PDVDS,nx,nz,flap,stallratio,netto
BOOL NMEAParser::PDVDS(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double flap, stallratio;
  swscanf(String,
	  TEXT("%lf,%lf,%lf,%lf,%lf"),
	  &GPS_INFO->AccelX,
	  &GPS_INFO->AccelZ,
	  &flap,
	  &stallratio,
	  &GPS_INFO->NettoVario);

  GPS_INFO->AccelX /= AccelerometerZero;
  GPS_INFO->AccelZ /= AccelerometerZero;
  int mag = isqrt4((int)((GPS_INFO->AccelX*GPS_INFO->AccelX
			  +GPS_INFO->AccelZ*GPS_INFO->AccelZ)*10000));
  GPS_INFO->Gload = mag/100.0;
  GPS_INFO->AccelerationAvailable = TRUE;
  GPS_INFO->NettoVarioAvailable = TRUE;
  GPS_INFO->NettoVario /= 10.0;

  if (EnableCalibration) {
    char buffer[200];
    sprintf(buffer,"%g %g %g %g %g %g #te net\r\n",
	    GPS_INFO->IndicatedAirspeed,
	    GPS_INFO->BaroAltitude,
	    GPS_INFO->Vario,
	    GPS_INFO->NettoVario,
	    GPS_INFO->AccelX,
	    GPS_INFO->AccelZ);
    DebugStore(buffer);
  }
  hasVega = true;

  return FALSE;
}


#include "VarioSound.h"

BOOL NMEAParser::PDAAV(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  unsigned short beepfrequency = (unsigned short)StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  unsigned short soundfrequency = (unsigned short)StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,2);
  unsigned char soundtype = (unsigned char)StrToDouble(ctemp, NULL);

	// Temporarily commented out - function as yet undefined
//  audio_setconfig(beepfrequency, soundfrequency, soundtype);

  return FALSE;
}

// $PDVDV,vario,ias,densityratio,altitude,staticpressure

BOOL NMEAParser::PDVDV(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  swscanf(String,
	  TEXT("%lf,%lf,%lf,%lf"),
	  &GPS_INFO->Vario, //
	  &GPS_INFO->IndicatedAirspeed,
	  &GPS_INFO->TrueAirspeed,
	  &GPS_INFO->BaroAltitude);

  GPS_INFO->Vario /= 10.0;
  GPS_INFO->VarioAvailable = TRUE;
  hasVega = true;

  GPS_INFO->IndicatedAirspeed /= 10.0;
  GPS_INFO->AirspeedAvailable = TRUE;
  GPS_INFO->TrueAirspeed *= GPS_INFO->IndicatedAirspeed/1024.0;
  GPS_INFO->BaroAltitudeAvailable = TRUE;

  VarioUpdated = TRUE;
  PulseEvent(varioTriggerEvent);

  return FALSE;
}


BOOL NMEAParser::PDVSC(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  TCHAR name[80];
  TCHAR responsetype[10];
  ExtractParameter(String,responsetype,0);
  ExtractParameter(String,name,1);
  ExtractParameter(String,ctemp,2);
  long value =  (long)StrToDouble(ctemp,NULL);
  DWORD dwvalue;

  TCHAR updatename[100];
  TCHAR fullname[100];
  _stprintf(updatename, TEXT("Vega%sUpdated"), name);
  _stprintf(fullname, TEXT("Vega%s"), name);
  SetToRegistry(updatename, 1);
  dwvalue = *((DWORD*)&value);
  SetToRegistry(fullname, dwvalue);

  /*
  wsprintf(ctemp,TEXT("%s"), &String[0]);
  DoStatusMessage(ctemp);
  */
  return FALSE;
}


BOOL NMEAParser::PDVVT(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  GPS_INFO->OutsideAirTemperature = StrToDouble(ctemp,NULL)/10.0-273.0;
  GPS_INFO->TemperatureAvailable = TRUE;

  ExtractParameter(String,ctemp,1);
  GPS_INFO->RelativeHumidity = StrToDouble(ctemp,NULL); // %
  GPS_INFO->HumidityAvailable = TRUE;

  return FALSE;
}


////////////// FLARM

void FLARM_RefreshSlots(NMEA_INFO *GPS_INFO) {
  int i;
  bool present = false;
  if (GPS_INFO->FLARM_Available) {

    for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
      // clear this slot if it is too old (2 seconds)
      if (_tcslen(GPS_INFO->FLARM_Traffic[i].ID)>0) {
	if (GPS_INFO->Time> GPS_INFO->FLARM_Traffic[i].Time_Fix+2) {
	  GPS_INFO->FLARM_Traffic[i].ID[0]= 0;
	} else {
	  present = true;
	}
      }
    }
  }
  GaugeFLARM::Show(present);
}


double FLARM_NorthingToLatitude = 0.0;
double FLARM_EastingToLongitude = 0.0;


BOOL NMEAParser::PFLAU(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  static int old_flarm_rx = 0;

  GPS_INFO->FLARM_Available = true;

  // calculate relative east and north projection to lat/lon

  double delta_lat = 0.01;
  double delta_lon = 0.01;

  double dlat = Distance(GPS_INFO->Latitude, GPS_INFO->Longitude,
			 GPS_INFO->Latitude+delta_lat, GPS_INFO->Longitude);

  FLARM_NorthingToLatitude = delta_lat / dlat;

  double dlon = Distance(GPS_INFO->Latitude, GPS_INFO->Longitude,
			 GPS_INFO->Latitude, GPS_INFO->Longitude+delta_lon);

  FLARM_EastingToLongitude = delta_lon / dlon;

  swscanf(String,
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
  if ((GPS_INFO->FLARM_RX==0) && (old_flarm_rx)) {
    // traffic has appeared..
    InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);
  }
  // XX: TODO also another event for new traffic.

  old_flarm_rx = GPS_INFO->FLARM_RX;

  return FALSE;
}


int FLARM_FindSlot(NMEA_INFO *GPS_INFO, TCHAR *Id)
{
  int i;
  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {

    // find position in existing slot
    if (_tcscmp(Id, GPS_INFO->FLARM_Traffic[i].ID)==0) {
      return i;
    }

    // find old empty slot

  }
  // not found, so try to find an empty slot
  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (GPS_INFO->FLARM_Traffic[i].ID[0]==0) {
      return i;
    }
  }

  // still not found and no empty slots left, buffer is full
  return -1;
}



BOOL NMEAParser::PFLAA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  int flarm_slot = 0;

  // 5 id, 6 digit hex
  ExtractParameter(String,ctemp,5);

  flarm_slot = FLARM_FindSlot(GPS_INFO, ctemp);
  if (flarm_slot<0) {
    // no more slots available,
    return FALSE;
  }

  _tcscpy(GPS_INFO->FLARM_Traffic[flarm_slot].ID, ctemp);

  // set time of fix to current time
  GPS_INFO->FLARM_Traffic[flarm_slot].Time_Fix = GPS_INFO->Time;

  swscanf(String,
	  TEXT("%hu,%lf,%lf,%lf,%hu,%6s,%lf,%lf,%lf,%lf,%hu"),
	  &GPS_INFO->FLARM_Traffic[flarm_slot].AlarmLevel, // unsigned short 0
	  &GPS_INFO->FLARM_Traffic[flarm_slot].RelativeNorth, // double?     1
	  &GPS_INFO->FLARM_Traffic[flarm_slot].RelativeEast, // double?      2
	  &GPS_INFO->FLARM_Traffic[flarm_slot].Altitude, // double           3
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
  GPS_INFO->FLARM_Traffic[flarm_slot].Altitude +=
    GPS_INFO->Altitude;

  return FALSE;
}


//////

void NMEAParser::TestRoutine(NMEA_INFO *GPS_INFO) {
  static int i=90;
  static TCHAR t1[] = TEXT("1,1,1,1");
  static TCHAR t2[] = TEXT("0,300,500,220,2,DD8F12,120,-4.5,30,-1.4,1");
  static TCHAR t3[] = TEXT("0,0,1200,50,2,DA8F12,120,-4.5,30,-1.4,1");
  //  static TCHAR t4[] = TEXT("-3,500,1024,50");

  i++;

  if (i>100) {
    i=0;
  }
  if (i<50) {
    GPS_INFO->FLARM_Available = true;
    nmeaParser1.PFLAU(t1,GPS_INFO);
    nmeaParser1.PFLAA(t2,GPS_INFO);
    nmeaParser1.PFLAA(t3,GPS_INFO);
  }
  //  nmeaParser1.PDVDV(t4,GPS_INFO);
}


///

bool EnableLogNMEA = false;
HANDLE nmeaLogFile = INVALID_HANDLE_VALUE;

void LogNMEA(TCHAR* text) {

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

