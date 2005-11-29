/*
  $Id: Parser.cpp,v 1.30 2005/11/29 06:12:17 jwharington Exp $

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

#include "parser.h"

extern bool EnableCalibration;

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

static BOOL PDVDS(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PDVDV(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PDAPL(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PDAAV(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PDVSC(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PDSWC(TCHAR *String, NMEA_INFO *GPS_INFO);
static BOOL PDVVT(TCHAR *String, NMEA_INFO *GPS_INFO);


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
BOOL VarioUpdated = TRUE;

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

      // JMW testing only
      /*
      if(_tcscmp(SentanceString,TEXT("PDAPL"))==0)
        {
	  DoStatusMessage(TEXT("Recv APL"));
          return FALSE;
        }
      */

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
    GPS_INFO->Latitude = tmplat;
    GPS_INFO->Longitude = tmplon;
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
    GPS_INFO->Latitude = tmplat;
    GPS_INFO->Longitude = tmplon;
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

  ExtractParameter(String,ctemp,6);
  GPS_INFO->SatellitesUsed = (int)(min(12,StrToDouble(ctemp, NULL)));

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
  GPS_INFO->IndicatedAirspeed = StrToDouble(ctemp, NULL)/TOKPH;
  // JMW TODO check, is this indicated or true airspeed?

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
  MACCREADY = StrToDouble(ctemp,NULL);
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

BOOL PBB50(TCHAR *String, NMEA_INFO *GPS_INFO)
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

  return FALSE;
}

double AccelerometerZero=100.0;

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
  GPS_INFO->AccelX = xval/AccelerometerZero;
  GPS_INFO->AccelZ = zval/AccelerometerZero;
  GPS_INFO->Gload = mag/AccelerometerZero;
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


BOOL PJV01(TCHAR *String, NMEA_INFO *GPS_INFO)
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

  return TRUE;
}

#include "InputEvents.h"

BOOL PDSWC(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  static unsigned long last_switchinputs;
  static unsigned long last_switchoutputs;

  ExtractParameter(String,ctemp,0);
  MACCREADY = StrToDouble(ctemp,NULL)/10;

  unsigned long switchinputs, switchoutputs;

  ExtractParameter(String,ctemp,1);
  switchinputs = wcstol(ctemp, NULL, 16);

  ExtractParameter(String,ctemp,2);
  switchoutputs = wcstol(ctemp, NULL, 16);

  //   airdata.circling = (switchoutputs && (1<<OUTPUT_BIT_CIRCLING));

  unsigned long up_switchinputs;
  unsigned long down_switchinputs;
  unsigned long up_switchoutputs;
  unsigned long down_switchoutputs;

  // detect changes to ON: on now (x) and not on before (!lastx)
  // detect changes to OFF: off now (!x) and on before (lastx)

  down_switchinputs = (switchinputs & (~last_switchinputs));
  up_switchinputs = ((~switchinputs) & (last_switchinputs));
  down_switchoutputs = (switchoutputs & (~last_switchoutputs));
  up_switchoutputs = ((~switchoutputs) & (last_switchoutputs));

  int i;
  unsigned long thebit;
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


// $PDVDS,nx,nz,flap,stallratio
BOOL PDVDS(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  GPS_INFO->AccelX = StrToDouble(ctemp,NULL)/AccelerometerZero;
  ExtractParameter(String,ctemp,1);
  GPS_INFO->AccelZ = StrToDouble(ctemp,NULL)/AccelerometerZero;
  int mag = isqrt4((int)((GPS_INFO->AccelX*GPS_INFO->AccelX
			  +GPS_INFO->AccelZ*GPS_INFO->AccelZ)*10000));
  GPS_INFO->Gload = mag/100.0;
  GPS_INFO->AccelerationAvailable = TRUE;

  ExtractParameter(String,ctemp,4);
  GPS_INFO->NettoVarioAvailable = TRUE;
  GPS_INFO->NettoVario = StrToDouble(ctemp,NULL)/10.0;

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

  return FALSE;
}


#include "VarioSound.h"

BOOL PDAAV(TCHAR *String, NMEA_INFO *GPS_INFO)
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

BOOL PDVDV(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];

  ExtractParameter(String,ctemp,0);
  GPS_INFO->Vario = StrToDouble(ctemp,NULL)/10.0;
  GPS_INFO->VarioAvailable = TRUE;

  ExtractParameter(String,ctemp,1);
  GPS_INFO->IndicatedAirspeed = StrToDouble(ctemp,NULL)/10.0;
  GPS_INFO->AirspeedAvailable = TRUE;

  ExtractParameter(String,ctemp,2);
  double densityratio = StrToDouble(ctemp,NULL)/1024.0;
  GPS_INFO->TrueAirspeed = GPS_INFO->IndicatedAirspeed*densityratio;

  ExtractParameter(String,ctemp,3);
  GPS_INFO->BaroAltitude = StrToDouble(ctemp,NULL);
  GPS_INFO->BaroAltitudeAvailable = TRUE;

  VarioUpdated = TRUE;

  return FALSE;
}


BOOL PDVSC(TCHAR *String, NMEA_INFO *GPS_INFO)
{
  TCHAR ctemp[80];
  wsprintf(ctemp,TEXT("%s"), &String[0]);
  DoStatusMessage(ctemp);
  return FALSE;
}


BOOL PDVVT(TCHAR *String, NMEA_INFO *GPS_INFO)
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
