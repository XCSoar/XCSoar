/*
  XCSoar Glide Computer
  Copyright (C) 2000 - 2004  M Roberts

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

  $Id: Parser.cpp,v 1.9 2005/07/07 15:08:13 jwharington Exp $
*/
#include "stdafx.h"
#include "parser.h"
#include "externs.h"
#include "utils.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>


#include <tchar.h>
#include "externs.h"
#include "VarioSound.h"

static BOOL GLL(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL GGA(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL RMC(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL RMB(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL RMZ(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL WP0(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL WP1(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL WP2(TCHAR *String, NMEA_INFO *GPS_INFO);

// Additional sentances added by JMW
static BOOL PJV01(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PBB50(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PBJVA(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PBJVH(TCHAR *String, NMEA_INFO *GPS_INFO);

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

BOOL GpsUpdated = TRUE;

BOOL ParseNMEAString(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  static TCHAR SentanceString[6] = TEXT("");
  static int i;

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
  
  if(String[0] != '$')
    {
      return FALSE;
    }

  if(!NMEAChecksum(String))
    {
      return FALSE;
    }

  if(String[1] == 'P')
    {
      //Proprietary String
      // JMW added his own variometer here...

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

      return FALSE;
    }
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

void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber)
{
  int index = 0;
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength        = 0;

  StringLength = _tcslen(Source);

  while( (CurrentFieldNumber < DesiredFieldNumber) && (index < StringLength) )
    {
      if ( Source[ index ] == ',' || Source[ index ] == '*' )
        {
          CurrentFieldNumber++;
        }

      index++;
    }

  if ( CurrentFieldNumber == DesiredFieldNumber )
    {
      while( (index < StringLength)    &&
             (Source[ index ] != ',') &&
             (Source[ index ] != '*') &&
             (Source[ index ] != '\0') )
        {
          Destination[dest_index] = Source[ index ];
          index++; dest_index++;
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

BOOL GLL(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80]; 
  double ThisTime;
  static double LastTime = 0;

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
    GPS_INFO->Lattitude = tmplat;
    GPS_INFO->Longditude = tmplon;
  }

  ExtractParameter(String,ctemp,5);
  GPS_INFO->NAVWarning = NAVWarn(ctemp[0]);

  return TRUE;
}

BOOL RMB(TCHAR *String, NMEA_INFO *GPS_INFO)
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

BOOL RMC(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  TCHAR *Stop;
  double ThisTime;
  static double LastTime = 0;

  ExtractParameter(String,ctemp,0);
  ThisTime = StrToDouble(ctemp, NULL);
  ThisTime = TimeModify(ThisTime);

  if (ThisTime>LastTime) {
    GpsUpdated = TRUE;
  }

  if(ThisTime<=LastTime)
    {
      LastTime = ThisTime;
      return FALSE;
    }

  ExtractParameter(String,ctemp,1);
  GPS_INFO->NAVWarning = NAVWarn(ctemp[0]);

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
    GPS_INFO->Lattitude = tmplat;
    GPS_INFO->Longditude = tmplon;
  }

  ExtractParameter(String,ctemp,6);
  GPS_INFO->Speed = KNOTSTOMETRESSECONDS * StrToDouble(ctemp, NULL);

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
  
  GPSCONNECT = TRUE;
  return TRUE;
}

BOOL GGA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  double ThisTime;
  static double LastTime = 0;

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
    GPS_INFO->Lattitude = tmplat;
    GPS_INFO->Longditude = tmplon;
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

  return TRUE;
}

BOOL RMZ(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  RMZAltitude = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  RMZAltitude = AltitudeModify(RMZAltitude,ctemp[0]);
  RMZAvailable = TRUE;
  return FALSE;
}

BOOL RMA(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  RMAAltitude = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  RMAAltitude = AltitudeModify(RMAAltitude,ctemp[0]);
  RMAAvailable = TRUE;
  return FALSE;
}

BOOL WP0(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
 
  GPS_INFO->BaroAltitudeAvailable = TRUE;
  ExtractParameter(String,ctemp,2);
  GPS_INFO->BaroAltitude = StrToDouble(ctemp, NULL);
  ExtractParameter(String,ctemp,1);
  GPS_INFO->Airspeed = StrToDouble(ctemp, NULL)/TOKPH;

  return FALSE;
}

BOOL WP1(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  return FALSE;
}


BOOL WP2(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  MACREADY = LIFTMODIFY*StrToDouble(ctemp,NULL);
  return FALSE;
}



BOOL NMEAChecksum(TCHAR *String)
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

BOOL PBB50(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  return FALSE;
}

BOOL PBJVA(TCHAR *String, NMEA_INFO *GPS_INFO)
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
  GPS_INFO->AccelX = xval/103.0;
  GPS_INFO->AccelZ = zval/103.0;
  GPS_INFO->Gload = mag/103.0;
  GPS_INFO->AccelerationAvailable = TRUE;

  return FALSE;
}



BOOL PBJVH(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double rh, oat;
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  rh = StrToDouble(ctemp,NULL)/100.0;
  ExtractParameter(String,ctemp,1);
  oat = StrToDouble(ctemp,NULL)/100.0;

  oat = oat/10.0-273.0;
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
  double rho_rat = sqrt(1.225/rho);
  return rho;
}


BOOL PJV01(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  double vias, wnet;
  TCHAR ctemp[80];
  int pstatic;
  TCHAR *Stop;

  ExtractParameter(String,ctemp,0);
  vias = StrToDouble(ctemp,NULL);
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
  GPS_INFO->Airspeed = vias/TOKNOTS;
  GPS_INFO->VarioAvailable = TRUE;
  GPS_INFO->Vario = wnet/TOKNOTS;

  return TRUE;
}
