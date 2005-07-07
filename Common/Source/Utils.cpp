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
*/
#include "stdafx.h"
#include "Utils.h"

#include "resource.h"
#include "externs.h"

#include "device.h"

#include "uniqueid.h"
#include "XCSoar.h"
#include "Topology.h"

#include <math.h>
#include <windows.h>
#include <tchar.h>

TCHAR szRegistryKey[] =                TEXT("Software\\MPSR\\XCSoar");
TCHAR *szRegistryDisplayType[] =     { TEXT("Info0"),
				       TEXT("Info1"),
				       TEXT("Info2"),
				       TEXT("Info3"),
				       TEXT("Info4"),
				       TEXT("Info5"),
				       TEXT("Info6"),
				       TEXT("Info7"),
				       TEXT("Info8"),
				       TEXT("Info9")
};
TCHAR *szRegistryColour[] =     { TEXT("Colour0"),
				  TEXT("Colour1"),
				  TEXT("Colour2"),
				  TEXT("Colour3"),
				  TEXT("Colour4"),
				  TEXT("Colour5"),
				  TEXT("Colour6"),
				  TEXT("Colour7"),
				  TEXT("Colour8"),
				  TEXT("Colour9"),
				  TEXT("Colour10"),
				  TEXT("Colour11"),
				  TEXT("Colour12")
};


TCHAR *szRegistryBrush[] =     {  TEXT("Brush0"),
				  TEXT("Brush1"),
				  TEXT("Brush2"),
				  TEXT("Brush3"),
				  TEXT("Brush4"),
				  TEXT("Brush5"),
				  TEXT("Brush6"),
				  TEXT("Brush7"),
				  TEXT("Brush8"),
				  TEXT("Brush9"),
				  TEXT("Brush10"),
				  TEXT("Brush11"),
				  TEXT("Brush12")
};


TCHAR szRegistryAirspaceWarning[]= TEXT("AirspaceWarn");
TCHAR szRegistryAirspaceBlackOutline[]= TEXT("AirspaceBlackOutline");
TCHAR szRegistryAltMargin[]=	   TEXT("AltMargin");
TCHAR szRegistryAltMode[]=  TEXT("AltitudeMode");
TCHAR szRegistryAltitudeUnitsValue[] = TEXT("Altitude");
TCHAR szRegistryCircleZoom[]= TEXT("CircleZoom");
TCHAR szRegistryClipAlt[]= TEXT("ClipAlt");
TCHAR szRegistryDisplayText[] = TEXT("DisplayText");
TCHAR szRegistryDisplayUpValue[] = TEXT("DisplayUp");
TCHAR szRegistryDistanceUnitsValue[] = TEXT("Distance");
TCHAR szRegistryDrawTerrain[]= TEXT("DrawTerrain");
TCHAR szRegistryDrawTopology[]= TEXT("DrawTopology");
TCHAR szRegistryFAISector[] = TEXT("FAISector");
TCHAR szRegistryFinalGlideTerrain[]= TEXT("FinalGlideTerrain");
TCHAR szRegistryHomeWaypoint[]= TEXT("HomeWaypoint");
TCHAR szRegistryLiftUnitsValue[] = TEXT("Lift");
TCHAR szRegistryPolarID[] = TEXT("Polar");
TCHAR szRegistryPort1Index[]= TEXT("PortIndex");
TCHAR szRegistryPort2Index[]=		 TEXT("Port2Index");
TCHAR szRegistryRegKey[]=				 TEXT("RegKey");
TCHAR szRegistrySafetyAltitudeArrival[] =     TEXT("SafetyAltitudeArrival");
TCHAR szRegistrySafetyAltitudeBreakOff[] =     TEXT("SafetyAltitudeBreakOff");
TCHAR szRegistrySafetyAltitudeTerrain[] =     TEXT("SafetyAltitudeTerrain");
TCHAR szRegistrySafteySpeed[] = TEXT("SafteySpeed");
TCHAR szRegistrySectorRadius[]=        TEXT("Radius");
TCHAR szRegistrySnailTrail[]=		 TEXT("SnailTrail");
TCHAR szRegistrySpeed1Index[]=		 TEXT("SpeedIndex");
TCHAR szRegistrySpeed2Index[]=		 TEXT("Speed2Index");
TCHAR szRegistrySpeedUnitsValue[] =    TEXT("Speed");
TCHAR szRegistryStartLine[]=		 TEXT("StartLine");
TCHAR szRegistryStartRadius[]=		 TEXT("StartRadius");
TCHAR szRegistryWarningTime[]=		 TEXT("WarnTime");
TCHAR szRegistryAcknowledgementTime[]=		 TEXT("AcknowledgementTime");
TCHAR szRegistryWindUpdateMode[] =       TEXT("WindUpdateMode");
TCHAR szRegistryWindSpeed[] =       TEXT("WindSpeed");
TCHAR szRegistryWindBearing[] =       TEXT("WindBearing");

TCHAR szRegistryAirfieldFile[]=  TEXT("AirfieldFile");
TCHAR szRegistryAirspaceFile[]=  TEXT("AirspaceFile");
TCHAR szRegistryPolarFile[] = TEXT("PolarFile");
TCHAR szRegistryTerrainFile[]=	 TEXT("TerrainFile");
TCHAR szRegistryTopologyFile[]=  TEXT("TopologyFile");
TCHAR szRegistryWayPointFile[]=  TEXT("WPFile");

TCHAR szRegistryPilotName[]=  TEXT("PilotName");
TCHAR szRegistryAircraftType[]=  TEXT("AircraftType");
TCHAR szRegistryAircraftRego[]=  TEXT("AircraftRego");

TCHAR szRegistrySoundVolume[]=  TEXT("SoundVolume");
TCHAR szRegistrySoundDeadband[]=  TEXT("SoundDeadband");
TCHAR szRegistrySoundAudioVario[]=  TEXT("AudioVario");
TCHAR szRegistrySoundTask[]=  TEXT("SoundTask");
TCHAR szRegistrySoundModes[]=  TEXT("SoundModes");
TCHAR szRegistryNettoSpeed[]= TEXT("NettoSpeed");

TCHAR szRegistryDeviceA[]= TEXT("DeviceA");
TCHAR szRegistryDeviceB[]= TEXT("DeviceB");

static double SINETABLE[910];
static float FSINETABLE[910];

void StoreType(int Index,int InfoType)
{
  SetToRegistry(szRegistryDisplayType[Index],(DWORD)InfoType);
}

void ReadRegistrySettings(void)
{
  DWORD Speed = 0;
  DWORD Distance = 0;
  DWORD Lift = 0;
  DWORD Altitude = 0;
  DWORD DisplayUp = 0;
  DWORD Temp = 0;
  int i;

  GetFromRegistry(szRegistrySpeedUnitsValue,&Speed);
  switch(Speed)
    {
    case 0 : SPEEDMODIFY = TOMPH;	break;
    case 1 : SPEEDMODIFY = TOKNOTS; break;
    case 2 : SPEEDMODIFY = TOKPH; break;
    }

  GetFromRegistry(szRegistryDistanceUnitsValue,&Distance);
  switch(Distance)
    {
    case 0 : DISTANCEMODIFY = TOMILES; break;
    case 1 : DISTANCEMODIFY = TONAUTICALMILES; break;
    case 2 : DISTANCEMODIFY = TOKILOMETER; break;
    }

  GetFromRegistry(szRegistryAltitudeUnitsValue,&Altitude);
  switch(Altitude)
    {
    case 0 : ALTITUDEMODIFY = TOFEET; break;
    case 1 : ALTITUDEMODIFY = TOMETER; break;
    }

  GetFromRegistry(szRegistryLiftUnitsValue,&Lift);
  switch(Lift)
    {
    case 0 : LIFTMODIFY = TOKNOTS; break;
    case 1 : LIFTMODIFY = TOMETER; break;
    }

  for (i=0;i<NUMINFOWINDOWS;i++)
    {
      if(GetFromRegistry(szRegistryDisplayType[i],&Altitude) == ERROR_SUCCESS )
	InfoType[i] = Altitude;
    }

  GetFromRegistry(szRegistryDisplayUpValue,&Temp);
  switch(Temp)
    {
    case TRACKUP : DisplayOrientation = TRACKUP; break;
    case NORTHUP : DisplayOrientation = NORTHUP;break;
    case NORTHCIRCLE : DisplayOrientation = NORTHCIRCLE;break;
    case TRACKCIRCLE : DisplayOrientation = TRACKCIRCLE;break;
    }

  GetFromRegistry(szRegistryDisplayText,&Temp);
  switch(Temp)
    {
    case 0 : DisplayTextType = DISPLAYNAME; break;
    case 1 : DisplayTextType = DISPLAYNUMBER;break;
    case 2 : DisplayTextType = DISPLAYFIRSTFIVE; break;
    case 3 : DisplayTextType = DISPLAYNONE;break;
    case 4 : DisplayTextType = DISPLAYFIRSTTHREE; break;
    case 5 : DisplayTextType = DISPLAYNAMEIFINTASK; break;
    }

  if(GetFromRegistry(szRegistryAltMode,&Temp)==ERROR_SUCCESS)
    AltitudeMode = Temp;

  if(GetFromRegistry(szRegistryClipAlt,&Temp)==ERROR_SUCCESS)
    ClipAltitude = Temp;

  if(GetFromRegistry(szRegistryAltMargin,&Temp)==ERROR_SUCCESS)
    AltWarningMargin = Temp;


  GetFromRegistry(szRegistrySafetyAltitudeArrival,&Temp);
  if(Temp != 0)
    SAFETYALTITUDEARRIVAL = (double)Temp;

  GetFromRegistry(szRegistrySafetyAltitudeBreakOff,&Temp);
  if(Temp != 0)
    SAFETYALTITUDEBREAKOFF = (double)Temp;

  GetFromRegistry(szRegistrySafetyAltitudeTerrain,&Temp);
  if(Temp != 0)
    SAFETYALTITUDETERRAIN = (double)Temp;

  GetFromRegistry(szRegistrySafteySpeed,&Temp);
  if(Temp != 0)
    SAFTEYSPEED = (double)Temp;

  Temp = 0;
  GetFromRegistry(szRegistryFAISector,&Temp);
  FAISector = Temp;

  GetFromRegistry(szRegistrySectorRadius,&SectorRadius);
  GetFromRegistry(szRegistryPolarID,&Temp); POLARID = (int)Temp;

  GetRegistryString(szRegistryRegKey, strRegKey, 65);

  for(i=0;i<12;i++)
    {
      if(GetFromRegistry(szRegistryBrush[i],&Temp)==ERROR_SUCCESS)
	iAirspaceBrush[i] =			(int)Temp;
      else
	iAirspaceBrush[i] =			0;

      if(GetFromRegistry(szRegistryColour[i],&Temp)==ERROR_SUCCESS)
	iAirspaceColour[i] =			(int)Temp;
      else
	iAirspaceColour[i] =			0;

      if (iAirspaceColour[i]>= NUMAIRSPACECOLORS) {
        iAirspaceColour[i]= 0;
      }

      if (iAirspaceBrush[i]>= NUMAIRSPACEBRUSHES) {
        iAirspaceBrush[i]= 0;
      }

    }

  GetFromRegistry(szRegistryAirspaceBlackOutline,&Temp);
  bAirspaceBlackOutline = Temp;

  GetFromRegistry(szRegistrySnailTrail,&Temp);
  TrailActive = Temp;

  GetFromRegistry(szRegistryDrawTopology,&Temp);
  EnableTopology = Temp;

  GetFromRegistry(szRegistryDrawTerrain,&Temp);
  EnableTerrain = Temp;

  GetFromRegistry(szRegistryFinalGlideTerrain,&Temp);
  FinalGlideTerrain = Temp;

  GetFromRegistry(szRegistryCircleZoom,&Temp);
  CircleZoom = Temp;

  GetFromRegistry(szRegistryWindUpdateMode,&Temp);
  WindUpdateMode = Temp;

  GetFromRegistry(szRegistryHomeWaypoint,&Temp);
  HomeWaypoint = Temp;

  Temp = 1;
  GetFromRegistry(szRegistryStartLine,&Temp);
  StartLine = Temp;

  Temp = 1000;
  GetFromRegistry(szRegistryStartRadius,&Temp);
  StartRadius = Temp;

  Temp = 0;
  GetFromRegistry(szRegistryAirspaceWarning,&Temp);
  AIRSPACEWARNINGS = Temp;

  Temp = 30;
  GetFromRegistry(szRegistryWarningTime,&Temp);
  WarningTime = Temp;

  Temp = 30;
  GetFromRegistry(szRegistryAcknowledgementTime,&Temp);
  AcknowledgementTime = Temp;

  Temp = 80;
  GetFromRegistry(szRegistrySoundVolume,&Temp);
  SoundVolume = Temp;

  Temp = 4;
  GetFromRegistry(szRegistrySoundDeadband,&Temp);
  SoundDeadband = Temp;

  Temp = 1;
  GetFromRegistry(szRegistrySoundAudioVario,&Temp);
  EnableSoundVario = Temp;

  Temp = 1;
  GetFromRegistry(szRegistrySoundTask,&Temp);
  EnableSoundTask = Temp;

  Temp = 1;
  GetFromRegistry(szRegistrySoundModes,&Temp);
  EnableSoundModes = Temp;

  Temp = 500;
  GetFromRegistry(szRegistryNettoSpeed,&Temp);
  NettoSpeed = Temp;

}



BOOL GetFromRegistry(const TCHAR *szRegValue, DWORD *pPos)
{
  HKEY    hKey;
  DWORD    dwSize, dwType;
  long    hRes;

  *pPos= 0;
  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, KEY_ALL_ACCESS, &hKey);
  if (hRes != ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return hRes;
    }

  dwSize = sizeof(DWORD);
  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);
  RegCloseKey(hKey);
  return hRes;
}


// Implement your code to save value to the regsitry

HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos)
{
  HKEY    hKey;
  DWORD    Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS)
    {
      return FALSE;
    }

  hRes = RegSetValueEx(hKey, szRegValue,0,REG_DWORD, (LPBYTE)&Pos, sizeof(DWORD));
  RegCloseKey(hKey);

  return hRes;
}

BOOL GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
  HKEY    hKey;
  DWORD   dwType = REG_SZ;
  long    hRes;
  unsigned int i;
  for (i=0; i<dwSize; i++) {
    pPos[i]=0;
  }

  pPos[0]= '\0';
  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, KEY_ALL_ACCESS, &hKey);
  if (hRes != ERROR_SUCCESS)
    {
      RegCloseKey(hKey);
      return FALSE;
    }

  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);
  RegCloseKey(hKey);
  return hRes;
}

HRESULT SetRegistryString(const TCHAR *szRegValue, TCHAR *Pos)
{
  HKEY    hKey;
  DWORD    Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szRegistryKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS)
    {
      return FALSE;
    }

  hRes = RegSetValueEx(hKey, szRegValue,0,REG_SZ, (LPBYTE)Pos, _tcslen(Pos)*sizeof(TCHAR));
  RegCloseKey(hKey);

  return hRes;
}

void ReadPort1Settings(DWORD *PortIndex, DWORD *SpeedIndex)
{
  DWORD Temp;

  if(GetFromRegistry(szRegistryPort1Index,&Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistry(szRegistrySpeed1Index,&Temp)==ERROR_SUCCESS)
    (*SpeedIndex) = Temp;
}


void WritePort1Settings(DWORD PortIndex, DWORD SpeedIndex)
{
  SetToRegistry(szRegistryPort1Index, PortIndex);
  SetToRegistry(szRegistrySpeed1Index, SpeedIndex);
}

void ReadPort2Settings(DWORD *PortIndex, DWORD *SpeedIndex)
{
  DWORD Temp;

  if(GetFromRegistry(szRegistryPort2Index,&Temp)==ERROR_SUCCESS)
    (*PortIndex) = Temp;

  if(GetFromRegistry(szRegistrySpeed2Index,&Temp)==ERROR_SUCCESS)
    (*SpeedIndex) = Temp;
}


void WritePort2Settings(DWORD PortIndex, DWORD SpeedIndex)
{
  SetToRegistry(szRegistryPort2Index, PortIndex);
  SetToRegistry(szRegistrySpeed2Index, SpeedIndex);
}

void rotate(double *xin, double *yin, double angle)
{
  static double x,y;
  static double xout, yout;
  static double lastangle = 0;
  static double cost=1,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = (double)fastcosine(angle);
      sint = (double)fastsine(angle);
    }

  x = *xin; y = *yin;
  xout = x*cost - y*sint;
  yout = y*cost + x*sint;
  *xin = xout;
  *yin = yout;
}


void frotate(float *xin, float *yin, float angle)
{
  static float x,y;
  static float xout, yout;
  static float lastangle = 0;
  static float cost=1,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = (float)fastcosine(angle);
      sint = (float)fastsine(angle);
    }

  x = *xin; y = *yin;
  xout = x*cost - y*sint;
  yout = y*cost + x*sint;
  *xin = xout;
  *yin = yout;
}


double Distance(double lat1, double lon1, double lat2, double lon2)
{
  double angle;

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;

  angle = (double)acos( sin(lat1)*sin(lat2) +
			cos(lat1)*cos(lat2) * cos(lon1-lon2)  )*RAD_TO_DEG;

  return (double)(angle * 111194.9267);
}


double Bearing(double lat1, double lon1, double lat2, double lon2)
{
  double angle;
  double d;

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;

  double clat1 = cos(lat1);
  double clat2 = cos(lat2);
  double slat1 = sin(lat1);
  double slat2 = sin(lat2);


  d = (slat1*slat2 +  clat1*clat2 * cos(lon1-lon2) );
  if(d>1) d = 0.99999999999999;
  if(d<-1) d = -0.99999999999999;
  d = acos(d);

  if(sin(lon1-lon2)<0 )
    {
      angle = (((slat2-slat1)
		* cos(d) ) / (sin(d)*clat1));

      if(angle >1) angle = 1;
      if(angle<-1) angle = -1;
      angle = acos(angle);

      /* JMW Redundant code?
      if(lat1>lat2)
	angle = angle * (180/pi);
      else
	angle = angle * (180/pi);
      */
      angle *= RAD_TO_DEG;
    }
  else
    {
      angle=(( (slat2-slat1)
	       * cos(d) ) / (sin(d)*clat1));
      if(angle >1) angle = 1;
      if(angle<-1) angle = -1;
      angle = acos(angle);

      angle = 360 - (angle * RAD_TO_DEG);
    }

  return (double)angle;
}


double Reciprocal(double InBound)
{
  if(InBound >= 180)
    return InBound - 180;
  else
    return InBound + 180;
}

double BiSector(double InBound, double OutBound)
{
  double result;


  InBound = Reciprocal(InBound);

  if(InBound == OutBound)
    {
      result = Reciprocal(InBound);
    }

  else if (InBound > OutBound)
    {
      if( (InBound - OutBound) < 180)
	{
	  result = Reciprocal((InBound+OutBound)/2);
	}
      else
	{
	  result = (InBound+OutBound)/2;
	}
    }
  else
    {
      if( (OutBound - InBound) < 180)
	{
	  result = Reciprocal((InBound+OutBound)/2);
	}
      else
	{
	  result = (InBound+OutBound)/2;
	}
    }
  return result;
}


/////////////////////////////////////////////////////////////////

void PolarWinPilot2XCSoar(double POLARV[3], double POLARW[3], double ww[2]) {
  double d;
  double v1,v2,v3;
  double w1,w2,w3;

  v1 = POLARV[0]/3.6; v2 = POLARV[1]/3.6; v3 = POLARV[2]/3.6;
  //	w1 = -POLARV[0]/POLARLD[0];
  //    w2 = -POLARV[1]/POLARLD[1];
  //    w3 = -POLARV[2]/POLARLD[2];
  w1 = POLARW[0]; w2 = POLARW[1]; w3 = POLARW[2];

  d = v1*v1*(v2-v3)+v2*v2*(v3-v1)+v3*v3*(v1-v2);
  if (d == 0.0)
    {
      POLAR[0]=0;
    }
  else
    {
      POLAR[0]=((v2-v3)*(w1-w3)+(v3-v1)*(w2-w3))/d;
    }
  d = v2-v3;
  if (d == 0.0)
    {
      POLAR[1]=0;
    }
  else
    {
      POLAR[1] = (w2-w3-POLAR[0]*(v2*v2-v3*v3))/d;
    }


  WEIGHTS[0] = 70;                      // Pilot weight
  WEIGHTS[1] = ww[0]-WEIGHTS[0];        // Glider empty weight
  WEIGHTS[2] = ww[1];                   // Ballast weight

  POLAR[2] = (double)(w3 - POLAR[0] *v3*v3 - POLAR[1]*v3);

  // now scale off weight
  POLAR[0] = POLAR[0] * (double)sqrt(WEIGHTS[0] + WEIGHTS[1]);
  POLAR[2] = POLAR[2] / (double)sqrt(WEIGHTS[0] + WEIGHTS[1]);

}




void PExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber)
{
  int index = 0;
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength        = 0;

  StringLength = _tcslen(Source);

  while( (CurrentFieldNumber < DesiredFieldNumber) && (index < StringLength) )
    {
      if ( Source[ index ] == ',' )
	{
	  CurrentFieldNumber++;
	}
      index++;
    }

  if ( CurrentFieldNumber == DesiredFieldNumber )
    {
      while( (index < StringLength)    &&
	     (Source[ index ] != ',') &&
	     (Source[ index ] != 0x00) )
	{
	  Destination[dest_index] = Source[ index ];
	  index++; dest_index++;
	}
      Destination[dest_index] = '\0';
    }
}


void ReadWinPilotPolar() {

  TCHAR	szFile[MAX_PATH] = TEXT("\0");
  TCHAR ctemp[80];
  TCHAR TempString[200];
  HANDLE hFile;

  double POLARV[3];
  double POLARW[3];
  double ww[2];

  ww[0]= 403.0; // 383
  ww[1]= 101.0; // 121
  POLARV[0]= 115.03;
  POLARW[0]= -0.86;
  POLARV[1]= 174.04;
  POLARW[1]= -1.76;
  POLARV[2]= 212.72;
  POLARW[2]= -3.4;

  GetRegistryString(szRegistryPolarFile, szFile, MAX_PATH);

  hFile = CreateFile(szFile,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile != INVALID_HANDLE_VALUE )
    {


      while(ReadString(hFile,200,TempString))
	{

	  if(_tcsstr(TempString,TEXT("*")) != TempString) // Look For Comment
	    {
	      PExtractParameter(TempString, ctemp, 0);
	      ww[0] = StrToDouble(ctemp,NULL);

	      PExtractParameter(TempString, ctemp, 1);
	      ww[1] = StrToDouble(ctemp,NULL);

	      PExtractParameter(TempString, ctemp, 2);
	      POLARV[0] = StrToDouble(ctemp,NULL);
	      PExtractParameter(TempString, ctemp, 3);
	      POLARW[0] = StrToDouble(ctemp,NULL);

	      PExtractParameter(TempString, ctemp, 4);
	      POLARV[1] = StrToDouble(ctemp,NULL);
	      PExtractParameter(TempString, ctemp, 5);
	      POLARW[1] = StrToDouble(ctemp,NULL);

	      PExtractParameter(TempString, ctemp, 6);
	      POLARV[2] = StrToDouble(ctemp,NULL);
	      PExtractParameter(TempString, ctemp, 7);
	      POLARW[2] = StrToDouble(ctemp,NULL);


	      PolarWinPilot2XCSoar(POLARV, POLARW, ww);

	    }
	}

      CloseHandle (hFile);
    }

}

// *LS-3	WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
//								403,				101,					115.03,		  -0.86,	174.04,	 -1.76,	212.72,	-3.4


//////////////////////////////////////////////////



void CalculateNewPolarCoef(void)
{
  static double Polars[7][3] =
    {
      {-0.0538770500225782443497, 0.1323114348, -0.1273364037098239098543},
      {-0.0532456270195884696748, 0.1509454717, -0.1474304674787072275183},
      {-0.0598306909918491529791, 0.1896480967, -0.1883344146619101871894},
      {-0.0303118230885946660507, 0.0771466019, -0.0799469636558217515699},
      {-0.0222929913566948641563, 0.0318771616, -0.0307925896846546928318},
      {-0.0430828898445299480353, 0.0746938776, -0.0487285153053357557183},
      {0.0, 0.0, 0.0}

    };


  /* Weights:
     0 Pilot Weight?
     1 Glider Weight
     2 BallastWeight
  */

  static double Weights[7][3] = { {70,190,1},
				  {70,250,100},
				  {70,240,285},
				  {70,287,165},  // w ok!
				  {70,400,120},  //
				  {70,527,303},
				  {0,0,0}
  };
  int i;

  for(i=0;i<3;i++)
    {
      POLAR[i] = Polars[POLARID][i];
      WEIGHTS[i] = Weights[POLARID][i];
    }
  if (POLARID==6) {
    ReadWinPilotPolar();
  }

}



int Registered(void)
{
  int Raw[8];
  int i;
  int Year, Month;

  _stscanf(strRegKey,TEXT("%02X%02X%02X%02X%02X%02X%02X%02X"),&Raw[0],&Raw[1],&Raw[2],&Raw[3],&Raw[4],&Raw[5],&Raw[6],&Raw[7]);

  for(i=0;i<8;i++)
    {
      Raw[i] = Raw[i] ^ (strAssetNumber[i] + i);
    }

  if(Raw[0] != Raw[4]) return FALSE;
  if(Raw[1] != Raw[5]) return FALSE;
  if(Raw[2] != Raw[6]) return FALSE;
  if(Raw[3] != Raw[7]) return FALSE;

  Month = MAKEWORD(Raw[0],Raw[1]);
  Year = MAKEWORD(Raw[2],Raw[3]);

  if(GPS_INFO.Year > Year) return FALSE;
  if((GPS_INFO.Year  == Year)&&(GPS_INFO.Month > Month)) return FALSE;

  return TRUE;
}

void CheckRegistration(void)
{
  static BOOL Warning = FALSE;
  static int RegistrationTimeOut = 150;
  static BOOL LOCK = FALSE;
  static int LastYear = 0, LastMonth = 0, LastDay = 0;


  if(LOCK)
    {
      return;
    }

  LOCK = TRUE;

  if((GPS_INFO.Year == 0) && (GPS_INFO.Month  == 0) && (GPS_INFO.Day  == 0))
    {
      LOCK = FALSE;
      LastYear = GPS_INFO.Year; LastMonth = GPS_INFO.Month; LastDay = GPS_INFO.Day;
      return;
    }

  if(GPS_INFO.Year <= 2004)
    {
      LOCK = FALSE;
      LastYear = GPS_INFO.Year; LastMonth = GPS_INFO.Month; LastDay = GPS_INFO.Day;

      return;
    }

  if((GPS_INFO.Year == 2003) && (GPS_INFO.Month  < 3))
    {
      LOCK = FALSE;
      LastYear = GPS_INFO.Year; LastMonth = GPS_INFO.Month; LastDay = GPS_INFO.Day;
      return;
    }

  if(Registered())
    {
      LastYear = GPS_INFO.Year; LastMonth = GPS_INFO.Month; LastDay = GPS_INFO.Day;
      LOCK = FALSE;
      return;
    }

  if((LastYear != GPS_INFO.Year)||(LastMonth != GPS_INFO.Month)||(LastDay = GPS_INFO.Day))
    {
      LastYear = GPS_INFO.Year; LastMonth = GPS_INFO.Month; LastDay = GPS_INFO.Day;
      LOCK = FALSE;
      return;
    }

  LastYear = GPS_INFO.Year; LastMonth = GPS_INFO.Month; LastDay = GPS_INFO.Day;

  if(Warning == FALSE)
    {
      MessageBox(hWndMainWindow,TEXT("Your Registration Key Has Expired Or No Registration Key Was Found.  See File->Register To Set A New Key.  Trial Use Limited To 10 Mins"),TEXT("Trial Use"),MB_OK|MB_ICONINFORMATION|MB_APPLMODAL|MB_TOPMOST);
      Warning = TRUE;
    }

  if(RegistrationTimeOut >0)
    {
      RegistrationTimeOut --;
      LOCK = FALSE;
      return;
    }
  else
    {
      MessageBox(hWndMainWindow,TEXT("Trial Period Has Expired.  See File->Register To Set A New Key"),TEXT("Trail Period Expired"),MB_OK|MB_ICONSTOP|MB_APPLMODAL|MB_TOPMOST);
      SendMessage(hWndMainWindow, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWndMainWindow);
      SendMessage (hWndMainWindow, WM_CLOSE, 0, 0);
    }
  LOCK = FALSE;
}

void CalculateTaskSectors(void)
{
  int i;
  double SectorAngle, SectorSize, SectorBearing;

  for(i=0;i<MAXTASKPOINTS-1;i++)
    {
      if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
	{
	  if(i == 0)
	    {
	      SectorAngle = 90;
	      SectorSize = StartRadius;
	      SectorBearing = Task[i].OutBound;
	    }
	  else
	    {
	      SectorAngle = 45;
	      SectorSize = 5000;
	      SectorBearing = Task[i].Bisector;
	    }

	  Task[i].SectorStartLat = FindLattitude(WayPointList[Task[i].Index].Lattitude,WayPointList[Task[i].Index].Longditude, SectorBearing + SectorAngle, SectorSize);
	  Task[i].SectorStartLon = FindLongditude(Task[i].SectorStartLat,WayPointList[Task[i].Index].Longditude, SectorBearing + SectorAngle, SectorSize);
	  Task[i].SectorEndLat   = FindLattitude(WayPointList[Task[i].Index].Lattitude,WayPointList[Task[i].Index].Longditude,	SectorBearing - SectorAngle, SectorSize);
	  Task[i].SectorEndLon   = FindLongditude(Task[i].SectorEndLat,WayPointList[Task[i].Index].Longditude,	SectorBearing - SectorAngle, SectorSize);
	}
    }
}

void CalculateAATTaskSectors(void)
{
  int i;

  if(AATEnabled == FALSE)
    return;

  for(i=1;i<MAXTASKPOINTS-1;i++)
    {
      if((Task[i].Index >=0))
	{
	  if(Task[i].AATType == SECTOR)
	    {
	      Task[i].AATStartLat = FindLattitude (WayPointList[Task[i].Index].Lattitude,WayPointList[Task[i].Index].Longditude, Task[i].AATStartRadial , Task[i].AATSectorRadius );
	      Task[i].AATStartLon = FindLongditude(WayPointList[Task[i].Index].Lattitude,WayPointList[Task[i].Index].Longditude, Task[i].AATStartRadial , Task[i].AATSectorRadius );
	      Task[i].AATFinishLat= FindLattitude (WayPointList[Task[i].Index].Lattitude,WayPointList[Task[i].Index].Longditude,	Task[i].AATFinishRadial , Task[i].AATSectorRadius );
	      Task[i].AATFinishLon= FindLongditude(WayPointList[Task[i].Index].Lattitude,WayPointList[Task[i].Index].Longditude,	Task[i].AATFinishRadial , Task[i].AATSectorRadius );
	    }
	}
    }
}


double FindLattitude(double Lat, double Lon, double Bearing, double Distance)
{
  double result;

  Lat *= DEG_TO_RAD;
  Lon *= DEG_TO_RAD;
  Bearing *= DEG_TO_RAD;

  Distance = Distance/6371000;

  result = (double)asin(sin(Lat)*cos(Distance)+cos(Lat)*sin(Distance)*cos(Bearing));
  result *= RAD_TO_DEG;
  return result;
}

double FindLongditude(double Lat, double Lon, double Bearing, double Distance)
{
  double result;

  Lat *= DEG_TO_RAD;
  Lon *= DEG_TO_RAD;
  Bearing *= DEG_TO_RAD;

  Distance = Distance/6371000;

  if(cos(Lat)==0)
    result = Lon;
  else
    {
      result = Lon+(double)asin(sin(Bearing)*sin(Distance)/cos(Lat));
      result = (double)fmod((result+pi),(2*pi));
      result = result - pi;
    }
  return result * RAD_TO_DEG;
}


static double xcoords[64] = {
  0,			0.09801714,		0.195090322,	0.290284677,	0.382683432,	0.471396737,	0.555570233,	0.634393284,
  0.707106781,	0.773010453,	0.831469612,	0.881921264,	0.923879533,	0.956940336,	0.98078528,		0.995184727,
  1,			0.995184727,	0.98078528,		0.956940336,	0.923879533,	0.881921264,	0.831469612,	0.773010453,
  0.707106781,	0.634393284,	0.555570233,	0.471396737,	0.382683432,	0.290284677,	0.195090322,	0.09801714,
  0,			-0.09801714,	-0.195090322,	-0.290284677,	-0.382683432,	-0.471396737,	-0.555570233,	-0.634393284,
  -0.707106781,	-0.773010453,	-0.831469612,	-0.881921264,	-0.923879533,	-0.956940336,	-0.98078528,	-0.995184727,
  -1,			-0.995184727,	-0.98078528,	-0.956940336,	-0.923879533,	-0.881921264,	-0.831469612,	-0.773010453,
  -0.707106781,	-0.634393284,	-0.555570233,	-0.471396737,	-0.382683432,	-0.290284677,	-0.195090322,	-0.09801714
};

static double ycoords[64] = {
  1,			0.995184727,	0.98078528,		0.956940336,	0.923879533,	0.881921264,	0.831469612,	0.773010453,
  0.707106781,	0.634393284,	0.555570233,	0.471396737,	0.382683432,	0.290284677,	0.195090322,	0.09801714,
  0,			-0.09801714,	-0.195090322,	-0.290284677,	-0.382683432,	-0.471396737,	-0.555570233,	-0.634393284,
  -0.707106781,	-0.773010453,	-0.831469612,	-0.881921264,	-0.923879533,	-0.956940336,	-0.98078528,	-0.995184727,
  -1,			-0.995184727,	-0.98078528,	-0.956940336,	-0.923879533,	-0.881921264,	-0.831469612,	-0.773010453,
  -0.707106781,	-0.634393284,	-0.555570233,	-0.471396737,	-0.382683432,	-0.290284677,	-0.195090322,	-0.09801714,
  0,			0.09801714,		0.195090322,	0.290284677,	0.382683432,	0.471396737,	0.555570233,	0.634393284,
  0.707106781,	0.773010453,	0.831469612,	0.881921264,	0.923879533,	0.956940336,	0.98078528,		0.995184727
};


int Circle(HDC hdc, long x, long y, int radius, RECT rc)
{
  POINT pt[65];
  int i;

  rectObj rect;
  rect.minx = x-radius;
  rect.maxx = x+radius;
  rect.miny = y-radius;
  rect.maxy = y+radius;
  rectObj rcrect;
  rcrect.minx = rc.left;
  rcrect.maxx = rc.right;
  rcrect.miny = rc.top;
  rcrect.maxy = rc.bottom;

  if (msRectOverlap(&rect, &rcrect)!=MS_TRUE) {
    return FALSE;
  }

  // JMW added faster checking...


  for(i=0;i<64;i++)
    {
      pt[i].x = x + (long) (radius * xcoords[i]);
      pt[i].y = y + (long) (radius * ycoords[i]);
    }
  pt[64].x = x + (long) (radius * xcoords[0]);
  pt[64].y = y + (long) (radius * ycoords[0]);

  Polygon(hdc,pt,65);
  return TRUE;
}


void ConvertFlightLevels(void)
{
  unsigned i;

  for(i=0;i<NumberOfAirspaceCircles;i++)
    {
      if(AirspaceCircle[i].Base.FL  != 0)
	{
	  AirspaceCircle[i].Base.Altitude = (AirspaceCircle[i].Base.FL * 100) + ((QNH-1013)*30);
	  AirspaceCircle[i].Base.Altitude = AirspaceCircle[i].Base.Altitude / TOFEET;
	}
      if(AirspaceCircle[i].Top.FL  != 0)
	{
	  AirspaceCircle[i].Top.Altitude = (AirspaceCircle[i].Top.FL * 100) + ((QNH-1013)*30);
	  AirspaceCircle[i].Top.Altitude = AirspaceCircle[i].Top.Altitude / TOFEET;
	}
    }


  for(i=0;i<NumberOfAirspaceAreas;i++)
    {
      if(AirspaceArea[i].Base.FL  != 0)
	{
	  AirspaceArea[i].Base.Altitude = (AirspaceArea[i].Base.FL * 100) + ((QNH-1013)*30);
	  AirspaceArea[i].Base.Altitude = AirspaceArea[i].Base.Altitude / TOFEET;
	}
      if(AirspaceArea[i].Top.FL  != 0)
	{
	  AirspaceArea[i].Top.Altitude = (AirspaceArea[i].Top.FL * 100) + ((QNH-1013)*30);
	  AirspaceArea[i].Top.Altitude = AirspaceArea[i].Top.Altitude / TOFEET;
	}
    }
}

// TODO: BUG
// NOTE THIS HAS HARD CODED SCREEN COORDINATES
BOOL PolygonVisible(const POINT *lpPoints, int nCount, RECT rc)
{
  BOOL Sector[9] = {FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};
  int i;
  int Count = 0;

  //return TRUE;

  for(i=0;i<nCount;i++)
    {
      if(lpPoints[i].y < 0)
	{
	  if(lpPoints[i].x <0)
	    {
	      Sector[0] = TRUE;
	    }
	  else if((lpPoints[i].x >=0) && (lpPoints[i].x <240))
	    {
	      Sector[1] = TRUE;
	    }
	  else if(lpPoints[i].x >=240)
	    {
	      Sector[2] = TRUE;
	    }
	}
      else if((lpPoints[i].y >=0) && (lpPoints[i].y <320))
	{
	  if(lpPoints[i].x <0)
	    {
	      Sector[3] = TRUE;
	    }
	  else if((lpPoints[i].x >=0) && (lpPoints[i].x <240))
	    {
	      Sector[4] = TRUE;
	      return TRUE;
	    }
	  else if(lpPoints[i].x >=240)
	    {
	      Sector[5] = TRUE;
	    }
	}
      else if(lpPoints[i].y >=320)
	{
	  if(lpPoints[i].x <0)
	    {
	      Sector[6] = TRUE;
	    }
	  else if((lpPoints[i].x >=0) && (lpPoints[i].x <240))
	    {
	      Sector[7] = TRUE;
	    }
	  else if(lpPoints[i].x >=240)
	    {
	      Sector[8] = TRUE;
	    }
	}
    }

  for(i=0;i<9;i++)
    {
      if(Sector[i])
	{
	  Count ++;
	}
    }

  if(Count>= 2)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

void SetRegistryColour(int i, DWORD c)
{
  SetToRegistry(szRegistryColour[i] ,c) ;
}


void SetRegistryBrush(int i, DWORD c)
{
  SetToRegistry(szRegistryBrush[i] ,c) ;
}


void ReadAssetNumber(void)
{
  if(strAssetNumber[0] != '\0')
    {
      return;
    }

  ReadUUID();

  if(strAssetNumber[0] != '\0')
    {
      return;
    }

  ReadCompaqID();
}

void ReadCompaqID(void)
{
  PROCESS_INFORMATION pi;
  HANDLE hInFile;// = INVALID_HANDLE_VALUE;
  DWORD dwBytesRead;

  if(strAssetNumber[0] != '\0')
    {
      return;
    }

  CreateProcess(TEXT("\\windows\\CreateAssetFile.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);

  hInFile = CreateFile(TEXT("\\windows\\cpqAssetData.dat"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hInFile == INVALID_HANDLE_VALUE)
    {
      //	    MessageBox(hWnd, TEXT("Unable to open asset data file."), TEXT("Error!"), MB_OK);
      return;
    }
  SetFilePointer(hInFile, 976, NULL, FILE_BEGIN);
  memset(strAssetNumber, 0, 64 * sizeof(TCHAR));
  ReadFile(hInFile, &strAssetNumber, 64, &dwBytesRead, NULL);
  CloseHandle(hInFile);
}

void ReadUUID(void)
{
  BOOL fRes;
  DWORD dwBytesReturned =0;
  DEVICE_ID* pDevID;
  int wSize;
  int i;

  GUID Guid;

  unsigned long temp, Asset;

  memset(&Guid, 0, sizeof(GUID));

  pDevID = (DEVICE_ID*)malloc(sizeof(DEVICE_ID));
  memset(pDevID, 0, sizeof(DEVICE_ID));
  pDevID->dwSize = sizeof(DEVICE_ID);

  fRes = KernelIoControl( IOCTL_HAL_GET_DEVICEID, NULL, 0,
			  pDevID, sizeof( DEVICE_ID ), &dwBytesReturned );

  wSize = pDevID->dwSize;
  free(pDevID); pDevID = NULL;

  if( (FALSE != fRes) || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
    return;

  pDevID = (DEVICE_ID*)malloc(sizeof(wSize));
  memset(pDevID, 0, sizeof(wSize));
  pDevID->dwSize = wSize;

  fRes = KernelIoControl( IOCTL_HAL_GET_DEVICEID, NULL, 0,
			  pDevID, wSize, &dwBytesReturned );

  if((FALSE == fRes) || (ERROR_INSUFFICIENT_BUFFER == GetLastError()) )
    return;

  BYTE* pDat = (BYTE*)&Guid.Data1;
  BYTE* pSrc = (BYTE*)(pDevID) + pDevID->dwPresetIDOffset;
  memcpy(pDat, pSrc, pDevID->dwPresetIDBytes);
  pDat +=  pDevID->dwPresetIDBytes;
  pSrc =  (BYTE*)(pDevID) + pDevID->dwPlatformIDOffset;
  memcpy(pDat, pSrc, pDevID->dwPlatformIDBytes);


  temp = Guid.Data2; temp = temp << 16;
  temp += Guid.Data3 ;

  Asset = temp ^ Guid.Data1 ;

  temp = 0;
  for(i=0;i<4;i++)
    {
      temp = temp << 8;
      temp += Guid.Data4[i];
    }

  Asset = Asset ^ temp;

  temp = 0;
  for(i=0;i<4;i++)
    {
      temp = temp << 8;
      temp += Guid.Data4[i+4];
    }

  Asset = Asset ^ temp;

  wsprintf(strAssetNumber,TEXT("%08X%08X"),Asset,Guid.Data1 );
  return;
}


void WriteFileRegistryString(HANDLE hFile, TCHAR *instring) {
    int len;
    char ctempFile[MAX_PATH];
    TCHAR tempFile[MAX_PATH];
    DWORD dwBytesWritten;
    int i;

    tempFile[0]=0;
    for (i=0; i<MAX_PATH; i++) {
      tempFile[i]= 0;
    }
    GetRegistryString(instring, tempFile, MAX_PATH);
    WideCharToMultiByte( CP_ACP, 0, tempFile,
			 _tcslen(tempFile)+1,
			 ctempFile,
			 MAX_PATH, NULL, NULL);
    for (i=0; i<MAX_PATH; i++) {
      if (ctempFile[i]=='\?') {
	ctempFile[i]=0;
      }
    }
    len = strlen(ctempFile)+1;
    ctempFile[len-1]= '\n';
    WriteFile(hFile,ctempFile,len, &dwBytesWritten,NULL);
}


void WriteProfile(HWND hwnd, TCHAR *szFile)
{
  HANDLE hFile;
  DWORD dwBytesWritten;

  hFile = CreateFile(szFile,GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES)NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile == INVALID_HANDLE_VALUE )
    {
      return;
    }

  WriteFile(hFile,InfoType,sizeof(InfoType),&dwBytesWritten,NULL);
  WriteFile(hFile,&POLARID,sizeof(POLARID),&dwBytesWritten,NULL);
  WriteFile(hFile,iAirspaceColour,11*sizeof(iAirspaceBrush[0]),&dwBytesWritten,NULL);
  WriteFile(hFile,iAirspaceBrush,11*sizeof(iAirspaceColour[0]),&dwBytesWritten,NULL);

  ///////


    WriteFileRegistryString(hFile, szRegistryAirfieldFile);
    WriteFileRegistryString(hFile, szRegistryAirspaceFile);
    WriteFileRegistryString(hFile, szRegistryPolarFile);
    WriteFileRegistryString(hFile, szRegistryTerrainFile);
    WriteFileRegistryString(hFile, szRegistryTopologyFile);
    WriteFileRegistryString(hFile, szRegistryWayPointFile);

    CloseHandle(hFile);
}


void ReadFileRegistryString(HANDLE hFile, TCHAR *instring) {
    int i;
    TCHAR tempFile[MAX_PATH];

    for (i=0; i<MAX_PATH; i++) {
      tempFile[i]= 0;
    }
    ReadString(hFile, MAX_PATH, tempFile);
    tempFile[_tcslen(tempFile)]= 0;
    SetRegistryString(instring, tempFile);
}


void ReadProfile(HWND hwnd, TCHAR *szFile)
{
  HANDLE hFile;
  DWORD dwBytesRead;
  int i;

  hFile = CreateFile(szFile,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile == INVALID_HANDLE_VALUE )
    {
      return;
    }


  ReadFile(hFile,InfoType,sizeof(InfoType),&dwBytesRead,NULL);
  for(i=0;i<NUMINFOWINDOWS;i++)
    {
      StoreType(i,InfoType[i]);
    }

  ReadFile(hFile,&POLARID,sizeof(POLARID),&dwBytesRead,NULL);
  CalculateNewPolarCoef();
  SetToRegistry(szRegistryPolarID,(DWORD)POLARID);

  ReadFile(hFile,iAirspaceColour,11*sizeof(iAirspaceColour[0]),&dwBytesRead,NULL);
  ReadFile(hFile,iAirspaceBrush,11*sizeof(iAirspaceBrush[0]),&dwBytesRead,NULL);
  for(i=0;i<11;i++)
    {

      SetRegistryColour(i,iAirspaceColour[i]);

      SetRegistryBrush(i,iAirspaceBrush[i]);
    }


  ///////

    ReadFileRegistryString(hFile, szRegistryAirfieldFile);
    ReadFileRegistryString(hFile, szRegistryAirspaceFile);
    ReadFileRegistryString(hFile, szRegistryPolarFile);
    ReadFileRegistryString(hFile, szRegistryTerrainFile);
    ReadFileRegistryString(hFile, szRegistryTopologyFile);
    ReadFileRegistryString(hFile, szRegistryWayPointFile);

    WAYPOINTFILECHANGED = TRUE;
    TERRAINFILECHANGED = TRUE;
    TOPOLOGYFILECHANGED = TRUE;
    AIRSPACEFILECHANGED = TRUE;
    AIRFIELDFILECHANGED = TRUE;
    // JMW TODO: WinPilot file changed

  CloseHandle(hFile);

  // assuming all is ok, we can...
  ReadRegistrySettings();

}


double ScreenAngle(int x1, int y1, int x2, int y2)
{
  return atan2(y2-y1, x2-x1)*RAD_TO_DEG;
}

void FormatWarningString(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top, TCHAR *szMessageBuffer, TCHAR *szTitleBuffer )
{
  TCHAR BaseStr[512];
  TCHAR TopStr[512];

  switch (Type)
    {
    case RESTRICT:		_tcscpy(szTitleBuffer,TEXT("Restricted")); break;
    case PROHIBITED:	_tcscpy(szTitleBuffer,TEXT("Prohibited")); break;
    case DANGER:			_tcscpy(szTitleBuffer,TEXT("Danger Area")); break;
    case CLASSA:			_tcscpy(szTitleBuffer,TEXT("Class A")); break;
    case CLASSB:			_tcscpy(szTitleBuffer,TEXT("Class B")); break;
    case CLASSC:			_tcscpy(szTitleBuffer,TEXT("Class C")); break;
    case CLASSD:			_tcscpy(szTitleBuffer,TEXT("Class D")); break;
    case NOGLIDER:		_tcscpy(szTitleBuffer,TEXT("No Glider")); break;
    case CTR:					_tcscpy(szTitleBuffer,TEXT("CTR")); break;
    case WAVE:				_tcscpy(szTitleBuffer,TEXT("Wave")); break;
    default:					_tcscpy(szTitleBuffer,TEXT("Unknown"));
    }

  if(Base.FL == 0)
    {
      wsprintf(BaseStr,TEXT("%1.0f Alt"),ALTITUDEMODIFY * Base.Altitude );
    }
  else
    {
      wsprintf(BaseStr,TEXT("FL %1.0f"),Base.FL );
    }

  if(Top.FL == 0)
    {
      wsprintf(TopStr,TEXT("%1.0f Alt"),ALTITUDEMODIFY * Top.Altitude );
    }
  else
    {
      wsprintf(TopStr,TEXT("FL %1.0f"),Top.FL );
    }

  wsprintf(szMessageBuffer,TEXT("%s - %s\r\nBase - %s\r\nTop - %s"),szTitleBuffer, Name, BaseStr, TopStr);
}

// read string from file
// support national codepage
// hFile:  file handle
// Max:    max chars to read from file
// String: pointer to string buffer
// return: True is at least one byte was read from file
//         False Max > 256 or EOF or read error
BOOL ReadString(HANDLE hFile, int Max, TCHAR *String)
{
  int i;
  char c;
  char sTmp[257];
  DWORD dwNumBytesRead;
  DWORD dwTotalNumBytesRead=0;

  String[0] = '\0';
  sTmp[0] = 0;

  if (Max >= sizeof(sTmp))
    return(FALSE);

  for(i=0;i<Max;i++){

    if (ReadFile(hFile,&c,1,&dwNumBytesRead,NULL)){

      dwTotalNumBytesRead += dwNumBytesRead;

      if(dwNumBytesRead != 0){

        if((c == '\n')){
          sTmp[i] = '\0';
          break;
        }

        sTmp[i] = c;
        continue;

      }

      sTmp[i] = 0;
      break;

    }
  }

  sTmp[Max-1] = '\0';

  mbstowcs(String, sTmp, strlen(sTmp)+1);

  return (dwTotalNumBytesRead>0);

}




void InitSineTable(void)
{
  int i;
  double angle;

  for(i=0;i<910;i++)
    {
      angle = 0.1 * (double)i;
      angle *= DEG_TO_RAD;
      SINETABLE[i] = (double)sin(angle);
      FSINETABLE[i] = (float)sin(angle);
    }
}


float ffastcosine(float x)
{
  return ffastsine(x+90);
}

double fastcosine(double x)
{
  return fastsine(x+90);
}

double fastsine(double x)
{
  int index;

  while(x<0)
    {
      x = x + 360;
    }
  while(x>=360)
    {
      x = x - 360;
    }
  index = (int)(x*10);
  if((index>=0 )&&(index<=900))
    {
      return SINETABLE[index];
    }
  else if((index>900)&&(index<=1800))
    {
      return SINETABLE[1800 - index];
    }
  else if((index>1800)&&(index<=2700))
    {
      return -1*SINETABLE[index-1800];
    }
  else if((index>2700)&&(index<=3600))
    {
      index = index - 1800;
      return -1*SINETABLE[1800-index];
    }
  else
    {
      return 0;
    }
}


float ffastsine(float x)
{
  int index;

  while(x<0)
    {
      x = x + 360;
    }
  while(x>=360)
    {
      x = x - 360;
    }
  index = (int)(x*10);
  if((index>=0 )&&(index<=900))
    {
      return FSINETABLE[index];
    }
  else if((index>900)&&(index<=1800))
    {
      return FSINETABLE[1800 - index];
    }
  else if((index>1800)&&(index<=2700))
    {
      return -FSINETABLE[index-1800];
    }
  else if((index>2700)&&(index<=3600))
    {
      index = index - 1800;
      return -FSINETABLE[1800-index];
    }
  else
    {
      return 0;
    }
}


double StrToDouble(TCHAR *Source, TCHAR **Stop)
{
  int index = 0;
  int StringLength        = 0;
  double Sum = 0;
  double Divisor = 10;
  int neg = 0;

  StringLength = _tcslen(Source);

  while((Source[index] == ' ')||(Source[index]==9))
    // JMW added skip for tab stop
    {
      index ++;
    }
  if (Source[index]=='-') {
    neg=1;
    index++;
  }

  while( (index < StringLength)
	 &&
	 (
	  (Source[index]>= '0') && (Source [index] <= '9')
          )
	 )
    {
      Sum = (Sum*10) + (Source[ index ] - '0');
      index ++;
    }
  if(Source[index] == '.')
    {
      index ++;
      while( (index < StringLength)
	     &&
	     (
	      (Source[index]>= '0') && (Source [index] <= '9')
	      )
	     )
	{
	  Sum = (Sum) + (double)(Source[ index ] - '0')/Divisor;
	  index ++;Divisor = Divisor * 10;
	}
    }
  if(Stop != NULL)
    *Stop = &Source[index];

  if (neg) {
    return -Sum;
  } else {
    return Sum;
  }
}



void SaveSoundSettings()
{
  SetToRegistry(szRegistrySoundVolume, (DWORD)SoundVolume);
  SetToRegistry(szRegistrySoundDeadband, (DWORD)SoundDeadband);
  SetToRegistry(szRegistrySoundAudioVario, (DWORD)EnableSoundVario);
  SetToRegistry(szRegistrySoundTask, (DWORD)EnableSoundTask);
  SetToRegistry(szRegistrySoundModes, (DWORD)EnableSoundModes);
}


void SaveWindToRegistry() {
  SetToRegistry(szRegistryWindSpeed,(DWORD)CALCULATED_INFO.WindSpeed);
  SetToRegistry(szRegistryWindBearing,(DWORD)CALCULATED_INFO.WindBearing);
}


void LoadWindFromRegistry() {
  DWORD Temp;
  GetFromRegistry(szRegistryWindSpeed,&Temp);
  CALCULATED_INFO.WindSpeed = (double)Temp;
  GetFromRegistry(szRegistryWindBearing,&Temp);
  CALCULATED_INFO.WindBearing = (double)Temp;

}

void ReadDeviceSettings(int devIdx, TCHAR *Name){

  Name[0] = '\0';

  if (devIdx == 0){
    GetRegistryString(szRegistryDeviceA , Name, DEVNAMESIZE);
    return;
  }

  if (devIdx == 1){
    GetRegistryString(szRegistryDeviceB , Name, DEVNAMESIZE);
    return;
  }

}

void WriteDeviceSettings(int devIdx, TCHAR *Name){

  if (devIdx == 0)
    SetRegistryString(szRegistryDeviceA , Name);

  if (devIdx == 1)
    SetRegistryString(szRegistryDeviceB , Name);
}

unsigned int isqrt4(unsigned long val) {
  unsigned int temp, g=0;

  if (val >= 0x40000000) {
    g = 0x8000;
    val -= 0x40000000;
  }

#define INNER_MBGSQRT(s)                      \
  temp = (g << (s)) + (1 << ((s) * 2 - 2));   \
  if (val >= temp) {                          \
    g += 1 << ((s)-1);                        \
    val -= temp;                              \
  }

  INNER_MBGSQRT (15)
  INNER_MBGSQRT (14)
  INNER_MBGSQRT (13)
  INNER_MBGSQRT (12)
  INNER_MBGSQRT (11)
  INNER_MBGSQRT (10)
  INNER_MBGSQRT ( 9)
  INNER_MBGSQRT ( 8)
  INNER_MBGSQRT ( 7)
  INNER_MBGSQRT ( 6)
  INNER_MBGSQRT ( 5)
  INNER_MBGSQRT ( 4)
  INNER_MBGSQRT ( 3)
  INNER_MBGSQRT ( 2)

#undef INNER_MBGSQRT

  temp = g+g+1;
  if (val >= temp) g++;
  return g;
}

// http://www.azillionmonkeys.com/qed/sqroot.html


int iround(double i) {
  int g=(int)i;
  if (fabs(i-g)>0.5) {
    if (i<0) {
      return g-1;
    } else {
      return g+1;
    }
  } else {
    return g;
  }
}

long lround(double i) {
  long g=(long)i;
  if (fabs(i-g)>0.5) {
    if (i<0) {
      return (g-1);
    } else {
      return (g+1);
    }
  } else {
    return g;
  }
}


static int ByteCRC16(int value, int crcin)
{
    int k = (((crcin >> 8) ^ value) & 255) << 8;
    int crc = 0;
    int bits = 8;
    do
    {
        if (( crc ^ k ) & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
        k <<= 1;
    }
    while (--bits);
    return ((crcin << 8) ^ crc);
}

WORD crcCalc(void *Buffer, size_t size){

  int crc = 0;
  unsigned char *pB = (unsigned char *)Buffer;

  do {
    int value = *pB++;
    crc = ByteCRC16(value, crc);
  } while (--size);

  return(crc);
}
