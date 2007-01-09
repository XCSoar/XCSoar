/*
  $Id: Parser.cpp,v 1.60 2007/01/09 01:53:56 jwharington Exp $

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
#include "device.h"
#include "Geoid.h"

extern bool EnableCalibration;

static double EastOrWest(double in, TCHAR EoW);
static double NorthOrSouth(double in, TCHAR NoS);
static double LeftOrRight(double in, TCHAR LoR);
static double AltitudeModify(double Altitude, TCHAR Format);
static double MixedFormatToDegrees(double mixed);
static double TimeModify(double FixTime, NMEA_INFO *gps_info);
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
  //hasVega = false;
  nSatellites = 0;

  activeGPS = false;
}

void NMEAParser::Reset(void) {
  // clear status
  nmeaParser1.gpsValid = false;
  nmeaParser2.gpsValid = false;
  nmeaParser1.activeGPS = true;
  nmeaParser2.activeGPS = true;
  //nmeaParser1.hasVega = false;
  //nmeaParser2.hasVega = false;

  // trigger updates
  GpsUpdated = TRUE;
  VarioUpdated = TRUE;
  SetEvent(dataTriggerEvent);
  PulseEvent(varioTriggerEvent);
}


int NMEAParser::FindVegaPort(void) {

  // hack, should be removed later if vega device driver is fully implemented

  if (_tcscmp(devA()->Name, TEXT("Vega")) == 0)
    return 0;
  if (_tcscmp(devB()->Name, TEXT("Vega")) == 0)
    return 1;

  /*
  if (nmeaParser1.hasVega)
    return 0;
  if (nmeaParser2.hasVega)
    return 1;
  */

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

      for(i=0;i<5;i++)
        {
          SentanceString[i] = String[i+1];
        }
      SentanceString[5] = '\0';

      if(_tcscmp(SentanceString,TEXT("PBB50"))==0)
        {
          return PBB50(&String[7], GPS_INFO);
        }

      // FLARM sentences
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

double TimeModify(double FixTime, NMEA_INFO* info)
{
  double hours, mins,secs;
  
  hours = FixTime / 10000;
  info->Hour = (int)hours;

  mins = FixTime / 100;
  mins = mins - (info->Hour*100);
  info->Minute = (int)mins;

  secs = FixTime - (info->Hour*10000) - (info->Minute*100);
  info->Second = (int)secs;

  FixTime = secs + (info->Minute*60) + (info->Hour*3600);

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
      InterfaceTimeoutReset();
      return TRUE;
    }

    GPS_INFO->NAVWarning = !gpsValid;

    ////
    
    ExtractParameter(String,ctemp,4);
    ThisTime = StrToDouble(ctemp,NULL);
    ThisTime = TimeModify(ThisTime, GPS_INFO);
    
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


bool SetSystemTimeFromGPS = false;


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
    ThisTime = TimeModify(ThisTime, GPS_INFO);

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

    // Altair doesn't have a battery-backed up realtime clock,
    // so as soon as we get a fix for the first time, set the
    // system clock to the GPS time.
    static bool sysTimeInitialised = false;

    if (!GPS_INFO->NAVWarning && (GPS_INFO->SatellitesUsed>3)) {
#ifdef GNAV
      SetSystemTimeFromGPS = true;
#endif
      if (SetSystemTimeFromGPS) {
	if (!sysTimeInitialised) {
	  
	  SYSTEMTIME sysTime;
	  ::GetSystemTime(&sysTime);
	  int hours = ((int)ThisTime)/3600;
	  int mins = ((int)ThisTime-hours*60)/60;
	  int secs = (int)ThisTime-hours*3600-mins*60;
	  sysTime.wYear = GPS_INFO->Year;
	  sysTime.wMonth = GPS_INFO->Month;
	  sysTime.wDay = GPS_INFO->Day;
	  sysTime.wHour = hours;
	  sysTime.wMinute = mins;
	  sysTime.wSecond = secs;
	  sysTime.wMilliseconds = 0;
	  sysTimeInitialised = (::SetSystemTime(&sysTime)==TRUE);

#ifdef GNAV
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
	// JMW changed from Altitude to BaroAltitude
	GPS_INFO->BaroAltitude = RMZAltitude;
      }
    else if(RMAAvailable)
	{
	// JMW changed from Altitude to BaroAltitude
	  GPS_INFO->BaroAltitude = RMAAltitude;
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
    ThisTime = TimeModify(ThisTime, GPS_INFO);
    
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
	// TODO: changed to BaroAltitude = ...
	GPS_INFO->BaroAltitude = RMZAltitude;
      }
    else if(RMAAvailable)
      {
	// TODO: changed to BaroAltitude = ...
	GPS_INFO->BaroAltitude = RMAAltitude;
      }
    else
      {
	// "Altitude" should always be GPS Altitude.
	ExtractParameter(String,ctemp,8);
	GPS_INFO->Altitude = StrToDouble(ctemp, NULL);
	ExtractParameter(String,ctemp,9);
	GPS_INFO->Altitude = AltitudeModify(GPS_INFO->Altitude,ctemp[0]);

	//
	double GeoidSeparation;
	ExtractParameter(String,ctemp,10);
	if (_tcslen(ctemp)>0) {
	  // No real need to parse this value,
	  // but we do assume that no correction is required in this case
	  GeoidSeparation = StrToDouble(ctemp, NULL);
	  ExtractParameter(String,ctemp,11);
	  GeoidSeparation = AltitudeModify(GeoidSeparation,ctemp[0]);
	} else {
	  // need to estimate Geoid Separation internally (optional)
      	  // FLARM uses MSL altitude
	  //
	  // Some others don't.
	  //
	  // If the separation doesn't appear in the sentence,
	  // we can assume the GPS unit is giving ellipsoid height
	  // 
	  GeoidSeparation = LookupGeoidSeparation(GPS_INFO->Latitude, 
						  GPS_INFO->Longitude);
	  GPS_INFO->Altitude -= GeoidSeparation;
	}

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


BOOL NMEAParser::PFLAU(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  static int old_flarm_rx = 0;

  GPS_INFO->FLARM_Available = true;

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
  if (GPS_INFO->FLARM_RX > old_flarm_rx) {
    // re-set suppression of gauge, as new traffic has arrived
    //    GaugeFLARM::Suppress = false;
  }
  if ((GPS_INFO->FLARM_RX==0) && (old_flarm_rx)) {
    // traffic has disappeared..
    InputEvents::processGlideComputer(GCE_FLARM_NOTRAFFIC);
  }
  // XX: TODO also another event for new traffic.

  old_flarm_rx = GPS_INFO->FLARM_RX;

  return FALSE;
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



BOOL NMEAParser::PFLAA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  int flarm_slot = 0;

  // 5 id, 6 digit hex
  ExtractParameter(String,ctemp,5);
  long ID;
  swscanf(ctemp,TEXT("%lx"), &ID);
  unsigned long uID = ID;

  flarm_slot = FLARM_FindSlot(GPS_INFO, ID);
  if (flarm_slot<0) {
    // no more slots available,
    return FALSE;
  }

  // set time of fix to current time
  GPS_INFO->FLARM_Traffic[flarm_slot].Time_Fix = GPS_INFO->Time;

  swscanf(String,
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

  TCHAR *name = GPS_INFO->FLARM_Traffic[flarm_slot].Name;
  if (!_tcslen(name)) {
    // need to lookup name for this target
    TCHAR *fname = LookupFLARMDetails(GPS_INFO->FLARM_Traffic[flarm_slot].ID);
    if (fname) {
      _tcscpy(name,fname);
    } else {
      name[0]=0;
    }
  }

  return FALSE;
}


//////

void NMEAParser::TestRoutine(NMEA_INFO *GPS_INFO) {
  static int i=90;
  static TCHAR t1[] = TEXT("1,1,1,1");
  static TCHAR t2[] = TEXT("0,300,500,220,2,DD8F12,120,-4.5,30,-1.4,1");
  static TCHAR t3[] = TEXT("0,0,1200,50,2,DA8B06,120,-4.5,30,-1.4,1");
  //  static TCHAR t4[] = TEXT("-3,500,1024,50");

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

