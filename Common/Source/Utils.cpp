/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
}
*/

#include "StdAfx.h"

#ifndef __MINGW32__
#if defined(CECORE)
#include "winbase.h"
#endif
#if (WINDOWSPC<1)
#include "projects.h"
#endif
#endif

#include "Utils.h"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Screen/Animation.hpp"
#include "Registry.hpp"
#include "resource.h"
#include "Dialogs.h"
#include "device.h"
#include "uniqueid.h"
#include "XCSoar.h"
#include "Topology.h"
#include "Terrain.h"
#include "Units.h"
#include "Calculations.h"
#include "GaugeFLARM.h"
#include "VegaVoice.h"
#include "McReady.h"
#include "NavFunctions.h"
#include "WaveThread.h" // VENTA4
#include "Compatibility/string.h"
#include "Screen/Util.hpp"
#include "Math/Pressure.h"
#include "Polar/WinPilot.hpp"
#include "Polar/BuiltIn.hpp"
#include "LocalPath.hpp"

#ifdef NEWFLARMDB
#include "FlarmIdFile.h"
FlarmIdFile file;
#endif

// VENTA2 added portrait settings in fontsettings for pnas
#if defined(PNA) || defined(FIVV)
#include "InfoBoxLayout.h"
#endif

#include <assert.h>

// JMW not required in newer systems?
#ifdef __MINGW32__
#ifndef max
#define max(x, y)   (x > y ? x : y)
#define min(x, y)   (x < y ? x : y)
#endif
#endif

int UTCOffset = 0; // used for Altair
bool LockSettingsInFlight = true;
bool LoggerShortName = false;


void ResetInfoBoxes(void) {
#ifdef GNAV
  InfoType[0]=873336334;
  InfoType[1]=856820491;
  InfoType[2]=822280982;
  InfoType[3]=2829105;
  InfoType[4]=103166000;
  InfoType[5]=421601569;
  InfoType[6]=657002759;
  InfoType[7]=621743887;
  InfoType[8]=439168301;
#else
  InfoType[0] = 921102;
  InfoType[1] = 725525;
  InfoType[2] = 262144;
  InfoType[3] = 74518;
  InfoType[4] = 657930;
  InfoType[5] = 2236963;
  InfoType[6] = 394758;
  InfoType[7] = 1644825;
#endif
}


void protate(POINT &pin, const double &angle)
{
  int x= pin.x;
  int y= pin.y;
  static double lastangle = 0;
  static int cost=1024,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = ifastcosine(angle);
      sint = ifastsine(angle);
    }
  pin.x = (x*cost - y*sint + 512 )/1024;
  pin.y = (y*cost + x*sint + 512 )/1024;

  // round (x/b) = (x+b/2)/b;
  // b = 2; x = 10 -> (10+1)/2=5
  // b = 2; x = 11 -> (11+1)/2=6
  // b = 2; x = -10 -> (-10+1)/2=4
}


void protateshift(POINT &pin, const double &angle,
                  const int &xs, const int &ys)
{
  int x= pin.x;
  int y= pin.y;
  static double lastangle = 0;
  static int cost=1024,sint=0;

  if(angle != lastangle)
    {
      lastangle = angle;
      cost = ifastcosine(angle);
      sint = ifastsine(angle);
    }
  pin.x = (x*cost - y*sint + 512 + (xs*1024))/1024;
  pin.y = (y*cost + x*sint + 512 + (ys*1024))/1024;

}

void PExtractParameter(const TCHAR *Source, TCHAR *Destination,
                       int DesiredFieldNumber)
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


//////////////////////////////////////////////////


typedef double PolarCoefficients_t[3];
typedef double WeightCoefficients_t[3];


void CalculateNewPolarCoef(void)
{

  StartupStore(TEXT("Calculate New Polar Coef\n"));

  static PolarCoefficients_t Polars[7] =
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

  static WeightCoefficients_t Weights[7] = { {70,190,1},
                                             {70,250,100},
                                             {70,240,285},
                                             {70,287,165},  // w ok!
                                             {70,400,120},  //
                                             {70,527,303},
                                             {0,0,0}
  };
  static double WingAreas[7] = {
    12.4,  // Ka6
    11.0,  // ASW19
    10.5,  // LS8
    9.0,   // ASW27
    11.4,  // LS6C-18
    16.31, // ASW22
    0};
  int i;

  assert(sizeof(Polars)/sizeof(Polars[0]) == sizeof(Weights)/sizeof(Weights[0]));

  if (POLARID < sizeof(Polars)/sizeof(Polars[0])){
    for(i=0;i<3;i++){
      POLAR[i] = Polars[POLARID][i];
      WEIGHTS[i] = Weights[POLARID][i];
    }
    GlidePolar::WingArea = WingAreas[POLARID];
  }
  if (POLARID==POLARUSEWINPILOTFILE) {
    if (ReadWinPilotPolar())
    // polar data gets from winpilot file
      return;
  } else if (POLARID>POLARUSEWINPILOTFILE){
    if (ReadWinPilotPolarInternal(POLARID-7))
      // polar data get from build in table
      return;
  } else if (POLARID<POLARUSEWINPILOTFILE){
    // polar data get from historical table
    return;
  }

  // ups
  // error reading winpilot file

  POLARID = 2;              // do it again with default polar (LS8)

  CalculateNewPolarCoef();
  MessageBoxX(gettext(TEXT("Error loading Polar file!\r\nUse LS8 Polar.")),
              gettext(TEXT("Warning")),
              MB_OK|MB_ICONERROR);

}

void ConvertFlightLevels(void)
{
  unsigned i;

  // TODO accuracy: Convert flightlevels is inaccurate!

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

BOOL PolygonVisible(const POINT *lpPoints, int nCount, RECT rc)
{
  BOOL Sector[9] = {FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};
  int i;
  int Count = 0;
  (void)rc;
  //return TRUE;

  for(i=0;i<nCount;i++)
    {
      if(lpPoints[i].y < MapWindow::MapRect.top)
	{
	  if(lpPoints[i].x < MapWindow::MapRect.left)
	    {
	      Sector[0] = TRUE;
	    }
	  else if((lpPoints[i].x >=MapWindow::MapRect.left)
		  && (lpPoints[i].x <MapWindow::MapRect.right))
	    {
	      Sector[1] = TRUE;
	    }
	  else if(lpPoints[i].x >=MapWindow::MapRect.right)
	    {
	      Sector[2] = TRUE;
	    }
	}
      else if((lpPoints[i].y >=MapWindow::MapRect.top)
	      && (lpPoints[i].y <MapWindow::MapRect.bottom))
	{
	  if(lpPoints[i].x <MapWindow::MapRect.left)
	    {
	      Sector[3] = TRUE;
	    }
	  else if((lpPoints[i].x >=MapWindow::MapRect.left)
		  && (lpPoints[i].x <MapWindow::MapRect.right))
	    {
	      Sector[4] = TRUE;
	      return TRUE;
	    }
	  else if(lpPoints[i].x >=MapWindow::MapRect.right)
	    {
	      Sector[5] = TRUE;
	    }
	}
      else if(lpPoints[i].y >=MapWindow::MapRect.bottom)
	{
	  if(lpPoints[i].x <MapWindow::MapRect.left)
	    {
	      Sector[6] = TRUE;
	    }
	  else if((lpPoints[i].x >=MapWindow::MapRect.left)
		  && (lpPoints[i].x <MapWindow::MapRect.right))
	    {
	      Sector[7] = TRUE;
	    }
	  else if(lpPoints[i].x >=MapWindow::MapRect.right)
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

void ReadAssetNumber(void)
{
  TCHAR val[MAX_PATH];

  val[0]= _T('\0');

  memset(strAssetNumber, 0, MAX_LOADSTRING*sizeof(TCHAR));
  // JMW clear this first just to be safe.

  StartupStore(TEXT("Asset ID: "));

#if (WINDOWSPC>0)
  return;
#endif

  GetRegistryString(szRegistryLoggerID, val, 100);
  int ifound=0;
  int len = _tcslen(val);
  for (int i=0; i< len; i++) {
    if (((val[i] >= _T('A'))&&(val[i] <= _T('Z')))
        ||((val[i] >= _T('0'))&&(val[i] <= _T('9')))) {
      strAssetNumber[ifound]= val[i];
      ifound++;
    }
    if (ifound>=3) {
      StartupStore(strAssetNumber);
      StartupStore(TEXT(" (reg)\n"));
      return;
    }
  }

  if(strAssetNumber[0] != '\0')
    {
      StartupStore(strAssetNumber);
      StartupStore(TEXT(" (?)\n"));
      return;
    }

  ReadCompaqID();
  if(strAssetNumber[0] != '\0')
    {
      StartupStore(strAssetNumber);
      StartupStore(TEXT(" (compaq)\n"));
      return;
    }

  ReadUUID();
  if(strAssetNumber[0] != '\0')
    {
      StartupStore(strAssetNumber);
      StartupStore(TEXT(" (uuid)\n"));
      return;
    }

  strAssetNumber[0]= _T('A');
  strAssetNumber[1]= _T('A');
  strAssetNumber[2]= _T('A');

  StartupStore(strAssetNumber);
  StartupStore(TEXT(" (fallback)\n"));

  return;
}

void ReadCompaqID(void)
{
  PROCESS_INFORMATION pi;

  if(strAssetNumber[0] != '\0')
    {
      return;
    }

  CreateProcess(TEXT("\\windows\\CreateAssetFile.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi);

  FILE *file = _tfopen(TEXT("\\windows\\cpqAssetData.dat"), TEXT("rb"));
  if (file == NULL)
    {
      //	    MessageBox(hWnd, TEXT("Unable to open asset data file."), TEXT("Error!"), MB_OK);
      return;
    }
  fseek(file, 976, SEEK_SET);
  memset(strAssetNumber, 0, 64 * sizeof(TCHAR));
  fread(&strAssetNumber, 64, 1, file);
  fclose(file);
}


void ReadUUID(void)
{
#if !(defined(__MINGW32__) && (WINDOWSPC>0))
  BOOL fRes;

#define GUIDBuffsize 100
  unsigned char GUIDbuffer[GUIDBuffsize];

  int eLast=0;
  int i;
  unsigned long uNumReturned=0;
  int iBuffSizeIn=0;
  unsigned long temp, Asset;


  GUID Guid;


  // approach followed: http://blogs.msdn.com/jehance/archive/2004/07/12/181116.aspx
  // 1) send 16 byte buffer - some older devices need this
  // 2) if buffer is wrong size, resize buffer accordingly and retry
  // 3) take first 16 bytes of buffer and process.  Buffer returned may be any size
  // First try exactly 16 bytes, some older PDAs require exactly 16 byte buffer

      #ifdef HAVEEXCEPTIONS
    __try {
      #else
	  strAssetNumber[0]= '\0';
      #endif

	  iBuffSizeIn=sizeof(Guid);
	  memset(GUIDbuffer, 0, iBuffSizeIn);
	  fRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, GUIDbuffer, iBuffSizeIn, &uNumReturned);
	  if(fRes == FALSE)
	  { // try larger buffer
		  eLast = GetLastError();
		  if (ERROR_INSUFFICIENT_BUFFER != eLast)
		  {
			return;
		  }
		  else
		  { // wrong buffer
			iBuffSizeIn = uNumReturned;
			memset(GUIDbuffer, 0, iBuffSizeIn);
			fRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, GUIDbuffer, iBuffSizeIn, &uNumReturned);
  			eLast = GetLastError();
			if(FALSE == fRes)
				return;
		  }
	  }

	  // here assume we have data in GUIDbuffer of length uNumReturned
	  memcpy(&Guid,GUIDbuffer, sizeof(Guid));


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

	  _stprintf(strAssetNumber,TEXT("%08X%08X"),Asset,Guid.Data1 );

#ifdef HAVEEXCEPTIONS
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
	  strAssetNumber[0]= '\0';
  }
#endif
#endif
  return;
}


#if 0
void ReadUUIDold(void)
{
#ifndef __MINGW32__
  BOOL fRes;
  DWORD dwBytesReturned =0;
  DEVICE_ID DevID;
  int wSize;
  int i;

  GUID Guid;

  unsigned long temp, Asset;

  memset(&Guid, 0, sizeof(GUID));

  memset(&DevID, 0, sizeof(DEVICE_ID));
  DevID.dwSize = sizeof(DEVICE_ID);

  fRes = KernelIoControl( IOCTL_HAL_GET_DEVICEID, NULL, 0,
			  &DevID, sizeof( DEVICE_ID ), &dwBytesReturned );

  wSize = DevID.dwSize;

  if( (FALSE != fRes) || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
    return;

  memset(&DevID, 0, sizeof(wSize));
  DevID.dwSize = wSize;

  fRes = KernelIoControl( IOCTL_HAL_GET_DEVICEID, NULL, 0,
			  &DevID, wSize, &dwBytesReturned );

  if((FALSE == fRes) || (ERROR_INSUFFICIENT_BUFFER == GetLastError()) )
    return;

  BYTE* pDat = (BYTE*)&Guid.Data1;
  BYTE* pSrc = (BYTE*)(&DevID) + DevID.dwPresetIDOffset;
  memcpy(pDat, pSrc, DevID.dwPresetIDBytes);
  pDat +=  DevID.dwPresetIDBytes;
  pSrc =  (BYTE*)(&DevID) + DevID.dwPlatformIDOffset;
  memcpy(pDat, pSrc, DevID.dwPlatformIDBytes);

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

  _stprintf(strAssetNumber,TEXT("%08X%08X"),Asset,Guid.Data1 );
  return;
#endif
}
#endif

#ifdef ENABLE_UNUSED_CODE
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
#ifdef _UNICODE
    WideCharToMultiByte( CP_ACP, 0, tempFile,
			 _tcslen(tempFile)+1,
			 ctempFile,
			 MAX_PATH, NULL, NULL);
#else
    strcpy(ctempFile, tempFile);
#endif
    for (i=0; i<MAX_PATH; i++) {
      if (ctempFile[i]=='\?') {
	ctempFile[i]=0;
      }
    }
    len = strlen(ctempFile)+1;
    ctempFile[len-1]= '\n';
    WriteFile(hFile,ctempFile,len, &dwBytesWritten, (OVERLAPPED *)NULL);
}
#endif /* ENABLE_UNUSED_CODE */

void WriteProfile(const TCHAR *szFile)
{
  SaveRegistryToFile(szFile);
}

#ifdef ENABLE_UNUSED_CODE
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
#endif /* ENABLE_UNUSED_CODE */

void ReadProfile(const TCHAR *szFile)
{
  LoadRegistryFromFile(szFile);

  WAYPOINTFILECHANGED = TRUE;
  TERRAINFILECHANGED = TRUE;
  TOPOLOGYFILECHANGED = TRUE;
  AIRSPACEFILECHANGED = TRUE;
  AIRFIELDFILECHANGED = TRUE;
  POLARFILECHANGED = TRUE;

  // assuming all is ok, we can...
  ReadRegistrySettings();
}


double ScreenAngle(int x1, int y1, int x2, int y2)
{
  return atan2((double)y2-y1, (double)x2-x1)*RAD_TO_DEG;
}

void FormatWarningString(int Type, const TCHAR *Name,
                         AIRSPACE_ALT Base, AIRSPACE_ALT Top,
                         TCHAR *szMessageBuffer, TCHAR *szTitleBuffer)
{
  TCHAR BaseStr[512];
  TCHAR TopStr[512];

  switch (Type)
    {
    case RESTRICT:
      _tcscpy(szTitleBuffer,gettext(TEXT("Restricted"))); break;
    case PROHIBITED:
      _tcscpy(szTitleBuffer,gettext(TEXT("Prohibited"))); break;
    case DANGER:
      _tcscpy(szTitleBuffer,gettext(TEXT("Danger Area"))); break;
    case CLASSA:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class A"))); break;
    case CLASSB:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class B"))); break;
    case CLASSC:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class C"))); break;
    case CLASSD:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class D"))); break;
    case CLASSE:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class E"))); break;
    case CLASSF:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class F"))); break;
    case NOGLIDER:
      _tcscpy(szTitleBuffer,gettext(TEXT("No Glider"))); break;
    case CTR:
      _tcscpy(szTitleBuffer,gettext(TEXT("CTR"))); break;
    case WAVE:
      _tcscpy(szTitleBuffer,gettext(TEXT("Wave"))); break;
    default:
      _tcscpy(szTitleBuffer,gettext(TEXT("Unknown")));
    }

  if(Base.FL == 0)
    {
      if (Base.AGL > 0) {
        _stprintf(BaseStr,TEXT("%1.0f%s %s"),
                  ALTITUDEMODIFY * Base.AGL,
                  Units::GetUnitName(Units::GetUserAltitudeUnit()),
                  gettext(TEXT("AGL")));
      } else if (Base.Altitude > 0)
        _stprintf(BaseStr,TEXT("%1.0f%s %s"),
                  ALTITUDEMODIFY * Base.Altitude,
                  Units::GetUnitName(Units::GetUserAltitudeUnit()),
                  gettext(TEXT("MSL")));
      else
        _stprintf(BaseStr,gettext(TEXT("GND")));
    }
  else
    {
      _stprintf(BaseStr,TEXT("FL %1.0f"),Base.FL );
    }

  if(Top.FL == 0)
    {
      if (Top.AGL > 0) {
        _stprintf(TopStr,TEXT("%1.0f%s %s"),
                  ALTITUDEMODIFY * Top.AGL,
                  Units::GetUnitName(Units::GetUserAltitudeUnit()),
                  gettext(TEXT("AGL")));
      } else {
	_stprintf(TopStr,TEXT("%1.0f%s %s"), ALTITUDEMODIFY * Top.Altitude,
		  Units::GetUnitName(Units::GetUserAltitudeUnit()),
		  gettext(TEXT("MSL")));
      }
    }
  else
    {
      _stprintf(TopStr,TEXT("FL %1.0f"),Top.FL );
    }

  _stprintf(szMessageBuffer,TEXT("%s: %s\r\n%s: %s\r\n%s: %s\r\n"),
            szTitleBuffer,
            Name,
            gettext(TEXT("Top")),
            TopStr,
            gettext(TEXT("Base")),
            BaseStr
            );
}


// JMW added support for zzip files

BOOL ReadString(ZZIP_FILE *zFile, int Max, TCHAR *String)
{
  char sTmp[READLINE_LENGTH+1];
  char FileBuffer[READLINE_LENGTH+1];
  long dwNumBytesRead=0;
  long dwTotalNumBytesRead=0;
  long dwFilePos;

  String[0] = '\0';
  sTmp[0] = 0;

  assert(Max<sizeof(sTmp));

  if (Max >= sizeof(sTmp))
    return(FALSE);
  if (!zFile)
    return(FALSE);

  dwFilePos = zzip_tell(zFile);

  dwNumBytesRead = zzip_fread(FileBuffer, 1, Max, zFile);
  if (dwNumBytesRead <= 0)
    return(FALSE);

  int i = 0;
  int j = 0;
  while((i<Max) && (j<(int)dwNumBytesRead)) {

    char c = FileBuffer[j];
    j++;
    dwTotalNumBytesRead++;

    if((c == '\n')){
      break;
    }

    sTmp[i] = c;
    i++;
  }

  sTmp[i] = 0;
  zzip_seek(zFile, dwFilePos+j, SEEK_SET);
  sTmp[Max-1] = '\0';
#ifdef _UNICODE
  mbstowcs(String, sTmp, strlen(sTmp)+1);
#else
  strcpy(String, sTmp);
#endif
  return (dwTotalNumBytesRead>0);
}

#ifdef ENABLE_UNUSED_CODE
// read string from file
// support national codepage
// hFile:  file handle
// Max:    max chars to fit in Buffer
// String: pointer to string buffer
// return: True if at least one byte was read from file
//         False Max > MAX_PATH or EOF or read error
BOOL ReadString(HANDLE hFile, int Max, TCHAR *String)
{
  char sTmp[READLINE_LENGTH+1];
  DWORD dwNumBytesRead=0;
  DWORD dwTotalNumBytesRead=0;
  char  FileBuffer[READLINE_LENGTH+1];
  DWORD dwFilePos;

  String[0] = '\0';
  sTmp[0] = 0;

  assert(Max<sizeof(sTmp));

  if (Max >= sizeof(sTmp))
    return(FALSE);

  dwFilePos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

  if (hFile == INVALID_HANDLE_VALUE)
    return(FALSE);

  if (ReadFile(hFile, FileBuffer, sizeof(FileBuffer),
	       &dwNumBytesRead, (OVERLAPPED *)NULL) == 0)
    return(FALSE);

  int i = 0;
  int j = 0;
  while(i<Max && j<(int)dwNumBytesRead){

    char c = FileBuffer[j];
    j++;
    dwTotalNumBytesRead++;

    if((c == '\n')){
      break;
    }

    sTmp[i] = c;
    i++;
    continue;
  }

  sTmp[i] = 0;
  SetFilePointer(hFile, dwFilePos+j, NULL, FILE_BEGIN);
  sTmp[Max-1] = '\0';
#ifdef _UNICODE
  mbstowcs(String, sTmp, strlen(sTmp)+1);
#else
  strcpy(String, sTmp);
#endif
  return (dwTotalNumBytesRead>0);

}
#endif /* ENABLE_UNUSED_CODE */

BOOL ReadStringX(FILE *fp, int Max, TCHAR *String){
  if (fp == NULL || Max < 1 || String == NULL) {
    if (String) {
      String[0]= '\0';
    }
    return (0);
  }

  if (_fgetts(String, Max, fp) != NULL){     // 20060512/sgi change 200 to max

    String[Max-1] = '\0';                    // 20060512/sgi added make shure the  string is terminated
    TCHAR *pWC = &String[max(0,_tcslen(String)-1)];
    // 20060512/sgi change add -1 to set pWC at the end of the string

    while (pWC > String && (*pWC == '\r' || *pWC == '\n')){
      *pWC = '\0';
      pWC--;
    }

    return (1);
  }

  return (0);

}



double StrToDouble(const TCHAR *Source, const TCHAR **Stop)
{
  int index = 0;
  int StringLength        = 0;
  double Sum = 0;
  double Divisor = 10;
  int neg = 0;

  StringLength = _tcslen(Source);

  while(((Source[index] == ' ')||(Source[index]=='+')||(Source[index]==9))
        && (index<StringLength))
    // JMW added skip for tab stop
    // JMW added skip for "+"
    {
      index ++;
    }
  if (index>= StringLength) {
    return 0.0; // error!
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
    *Stop = (TCHAR *)&Source[index];

  if (neg) {
    return -Sum;
  } else {
    return Sum;
  }
}


// RMN: Volkslogger outputs data in hex-strings.  Function copied from StrToDouble
// Note: Decimal-point and decimals disregarded.  Assuming integer amounts only.
double HexStrToDouble(TCHAR *Source, TCHAR **Stop)
{
  int index = 0;
  int StringLength        = 0;
  double Sum = 0;
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

  while(
  (index < StringLength)	 &&
	(	( (Source[index]>= '0') && (Source [index] <= '9')  ) ||
		( (Source[index]>= 'A') && (Source [index] <= 'F')  ) ||
		( (Source[index]>= 'a') && (Source [index] <= 'f')  )
		)
	)
    {
      if((Source[index]>= '0') && (Source [index] <= '9'))	  {
		Sum = (Sum*16) + (Source[ index ] - '0');
		index ++;
	  }
	  if((Source[index]>= 'A') && (Source [index] <= 'F'))	  {
		Sum = (Sum*16) + (Source[ index ] - 'A' + 10);
		index ++;
	  }
	  if((Source[index]>= 'a') && (Source [index] <= 'f'))	  {
		Sum = (Sum*16) + (Source[ index ] - 'a' + 10);
		index ++;
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

#ifdef ENABLE_UNUSED_CODE
WORD crcCalc(void *Buffer, size_t size){

  int crc = 0;
  unsigned char *pB = (unsigned char *)Buffer;

  do {
    int value = *pB++;
    crc = ByteCRC16(value, crc);
  } while (--size);

  return((WORD)crc);
}
#endif /* ENABLE_UNUSED_CODE */

///////////

void ExtractDirectory(TCHAR *Dest, const TCHAR *Source) {
  int len = _tcslen(Source);
  int found = -1;
  int i;
  if (len==0) {
    Dest[0]= 0;
    return;
  }
  for (i=0; i<len; i++) {
    if ((Source[i]=='/')||(Source[i]=='\\')) {
      found = i;
    }
  }
  for (i=0; i<=found; i++) {
    Dest[i]= Source[i];
  }
  Dest[i]= 0;
}


/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. [rescinded 22 July 1999]
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Perform a binary search.
 *
 * The code below is a bit sneaky.  After a comparison fails, we
 * divide the work in half by moving either left or right. If lim
 * is odd, moving left simply involves halving lim: e.g., when lim
 * is 5 we look at item 2, so we change lim to 2 so that we will
 * look at items 0 & 1.  If lim is even, the same applies.  If lim
 * is odd, moving right again involes halving lim, this time moving
 * the base up one item past p: e.g., when lim is 5 we change base
 * to item 3 and make lim 2 so that we will look at items 3 and 4.
 * If lim is even, however, we have to shrink it by one before
 * halving: e.g., when lim is 4, we still looked at item 2, so we
 * have to make lim 3, then halve, obtaining 1, so that we will only
 * look at item 3.
 */

void *bsearch(void *key, void *base0, size_t nmemb, size_t size, int (*compar)(const void *elem1, const void *elem2)){
	void *base = base0;
	int lim, cmp;
	void *p;

	for (lim = nmemb; lim != 0; lim >>= 1) {
		p = (char *)base + (lim >> 1) * size;
		cmp = (*compar)(key, p);
		if (cmp == 0)
			return (p);
		if (cmp > 0) {	/* key > p: move right */
			base = (char *)p + size;
			lim--;
		} /* else move left */
	}
	return (NULL);
}



TCHAR *strtok_r(TCHAR *s, const TCHAR *delim, TCHAR **lasts){
// "s" MUST be a pointer to an array, not to a string!!!
// (ARM92, Win emulator cause access violation if not)

  TCHAR *spanp;
	int   c, sc;
	TCHAR *tok;


	if (s == NULL && (s = *lasts) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */

cont:
	c = *s++;
	for (spanp = (TCHAR *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*lasts = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (TCHAR *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;  // causes access violation in some configs if s is a pointer instead of an array
				*lasts = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

TCHAR *
StringMallocParse(const TCHAR* old_string)
{
  TCHAR buffer[2048];	// Note - max size of any string we cope with here !
  TCHAR* new_string;
  unsigned int used = 0;
  unsigned int i;
  for (i = 0; i < _tcslen(old_string); i++) {
    if (used < 2045) {
      if (old_string[i] == '\\' ) {
        if (old_string[i + 1] == 'r') {
          buffer[used++] = '\r';
          i++;
        } else if (old_string[i + 1] == 'n') {
          buffer[used++] = '\n';
          i++;
        } else if (old_string[i + 1] == '\\') {
          buffer[used++] = '\\';
          i++;
        } else {
          buffer[used++] = old_string[i];
        }
      } else {
	buffer[used++] = old_string[i];
      }
    }
  };
  buffer[used++] =_T('\0');

  new_string = (TCHAR *)malloc((_tcslen(buffer)+1)*sizeof(TCHAR));
  _tcscpy(new_string, buffer);

  return new_string;
}

void ConvertTToC(CHAR* pszDest, const TCHAR* pszSrc)
{
	for(unsigned int i = 0; i < _tcslen(pszSrc); i++)
		pszDest[i] = (CHAR) pszSrc[i];
}

void ConvertCToT(TCHAR* pszDest, const CHAR* pszSrc)
{
	for(unsigned int i = 0; i < strlen(pszSrc); i++)
		pszDest[i] = (TCHAR) pszSrc[i];
}


void propGetFontSettingsFromString(const TCHAR *Buffer1, LOGFONT* lplf)
{
#define propGetFontSettingsMAX_SIZE 128
  TCHAR Buffer[propGetFontSettingsMAX_SIZE]; // RLD need a buffer (not sz) for strtok_r w/ gcc optimized ARM920

  TCHAR *pWClast, *pToken;
  LOGFONT lfTmp;
  _tcsncpy(Buffer, Buffer1, propGetFontSettingsMAX_SIZE);
    // FontDescription of format:
    // typical font entry
    // 26,0,0,0,700,1,0,0,0,0,0,4,2,<fontname>

    //FW_THIN   100
    //FW_NORMAL 400
    //FW_MEDIUM 500
    //FW_BOLD   700
    //FW_HEAVY  900

  assert(lplf != NULL);
  memset ((void *)&lfTmp, 0, sizeof (LOGFONT));

  if ((pToken = strtok_r(Buffer, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfHeight = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfWidth = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfEscapement = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfOrientation = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfWeight = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfItalic = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfUnderline = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfStrikeOut = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfCharSet = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfOutPrecision = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfClipPrecision = (unsigned char)_tcstol(pToken, NULL, 10);

  // DEFAULT_QUALITY			   0
  // RASTER_FONTTYPE			   0x0001
  // DRAFT_QUALITY			     1
  // NONANTIALIASED_QUALITY  3
  // ANTIALIASED_QUALITY     4
  // CLEARTYPE_QUALITY       5
  // CLEARTYPE_COMPAT_QUALITY 6

  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfQuality = (unsigned char)_tcstol(pToken, NULL, 10);

  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfPitchAndFamily = (unsigned char)_tcstol(pToken, NULL, 10);

  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;

  _tcscpy(lfTmp.lfFaceName, pToken);

  memcpy((void *)lplf, (void *)&lfTmp, sizeof (LOGFONT));

  return;
}


void propGetFontSettings(const TCHAR *Name, LOGFONT* lplf) {

  TCHAR Buffer[128];

  assert(Name != NULL);
  assert(Name[0] != '\0');
  assert(lplf != NULL);

#if (WINDOWSPC>0) && !defined(PCGNAV)
  // Don't load font settings from registry values for windows version
  return;
#endif

  if (GetRegistryString(Name, Buffer, sizeof(Buffer)/sizeof(TCHAR)) == 0) {
    propGetFontSettingsFromString(Buffer, lplf);
  }
}


int propGetScaleList(double *List, size_t Size){

  TCHAR Buffer[128];
  TCHAR Name[] = TEXT("ScaleList");
  TCHAR *pWClast, *pToken;
  int   Idx = 0;
  double vlast=0;
  double val;

  assert(List != NULL);
  assert(Size > 0);

  SetRegistryString(TEXT("ScaleList"),
   TEXT("0.5,1,2,5,10,20,50,100,150,200,500,1000"));

  if (GetRegistryString(Name, Buffer, sizeof(Buffer)/sizeof(TCHAR)) == 0){

    pToken = strtok_r(Buffer, TEXT(","), &pWClast);

    while(Idx < (int)Size && pToken != NULL){
      val = _tcstod(pToken, NULL);
      if (Idx>0) {
        List[Idx] = (val+vlast)/2;
        Idx++;
      }
      List[Idx] = val;
      Idx++;
      vlast = val;
      pToken = strtok_r(NULL, TEXT(","), &pWClast);
    }

    return(Idx);

  } else {
    return(0);
  }

}


long GetUTCOffset(void) {
#ifndef GNAV
  long utcoffset=0;
  // returns offset in seconds
  TIME_ZONE_INFORMATION TimeZoneInformation;
  DWORD tzi = GetTimeZoneInformation(&TimeZoneInformation);

  utcoffset = -TimeZoneInformation.Bias*60;

  if (tzi==TIME_ZONE_ID_STANDARD) {
    utcoffset -= TimeZoneInformation.StandardBias*60;
  }
  if (tzi==TIME_ZONE_ID_DAYLIGHT) {
    utcoffset -= TimeZoneInformation.DaylightBias*60;
  }
#if (WINDOWSPC>0)
  return UTCOffset;
#else
  return utcoffset;
#endif
#else
  return UTCOffset;
#endif
}


int TextToLineOffsets(TCHAR* text, int* LineOffsets, int maxLines) {
  int nTextLines=0;
  LineOffsets[0]= 0;
  if (text) {
    if (_tcslen(text)>0) {

      int delta = 0;
      int cumul = 0;
      TCHAR* vind = text;

      while (nTextLines<maxLines) {
	delta = _tcscspn(vind+cumul, TEXT("\n"));
	if (!delta) {
	  break;
	}
	if (_tcslen(vind+cumul+delta)>0) {
	  delta++;
	} else {
	  break;
	}
	cumul += delta;
	nTextLines++;
	LineOffsets[nTextLines]= cumul;
      }
      nTextLines++;

    }
  }
  return nTextLines;
}


/////////


TCHAR startProfileFile[MAX_PATH];
TCHAR defaultProfileFile[MAX_PATH];
TCHAR failsafeProfileFile[MAX_PATH];

void RestoreRegistry(void) {
  StartupStore(TEXT("Restore registry\n"));
  // load registry backup if it exists
  LoadRegistryFromFile(failsafeProfileFile);
  LoadRegistryFromFile(startProfileFile);
}

void StoreRegistry(void) {
  StartupStore(TEXT("Store registry\n"));
  // save registry backup first (try a few places)
  SaveRegistryToFile(startProfileFile);
  SaveRegistryToFile(defaultProfileFile);
}

void XCSoarGetOpts(LPTSTR CommandLine) {
  (void)CommandLine;
// SaveRegistryToFile(TEXT("iPAQ File Store\xcsoar-registry.prf"));

#ifdef GNAV
  LocalPath(defaultProfileFile,TEXT("config/xcsoar-registry.prf"));
#else
  //LocalPath(defaultProfileFile,TEXT("xcsoar-registry.prf"));
  LocalPath(defaultProfileFile,TEXT(XCSPROFILE)); // VENTA4
#endif
  // LocalPath(failsafeProfileFile,TEXT("xcsoar-registry.prf")); VENTA4
  LocalPath(failsafeProfileFile,TEXT(XCSPROFILE));
  _tcscpy(startProfileFile, defaultProfileFile);

#if (WINDOWSPC>0)
  SCREENWIDTH=640;
  SCREENHEIGHT=480;

#if defined(SCREENWIDTH_)
  SCREENWIDTH=SCREENWIDTH_;
#endif
#if defined(SCREENHEIGHT_)
  SCREENHEIGHT=SCREENHEIGHT_;
#endif

#else
  return; // don't do anything for PDA platforms
#endif

  TCHAR *MyCommandLine = GetCommandLine();

  if (MyCommandLine != NULL){
    TCHAR *pC, *pCe;

    pC = _tcsstr(MyCommandLine, TEXT("-profile="));
    if (pC != NULL){
      pC += strlen("-profile=");
      if (*pC == '"'){
        pC++;
        pCe = pC;
        while (*pCe != '"' && *pCe != '\0') pCe++;
      } else{
        pCe = pC;
        while (*pCe != ' ' && *pCe != '\0') pCe++;
      }
      if (pCe != NULL && pCe-1 > pC){

        _tcsncpy(startProfileFile, pC, pCe-pC);
        startProfileFile[pCe-pC] = '\0';
      }
    }
#if (WINDOWSPC>0)
    pC = _tcsstr(MyCommandLine, TEXT("-800x480"));
    if (pC != NULL){
      SCREENWIDTH=800;
      SCREENHEIGHT=480;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-480x272"));
    if (pC != NULL){
      SCREENWIDTH=480;
      SCREENHEIGHT=272;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-480x234"));
    if (pC != NULL){
      SCREENWIDTH=480;
      SCREENHEIGHT=234;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-portrait"));
    if (pC != NULL){
      SCREENWIDTH=480;
      SCREENHEIGHT=640;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-square"));
    if (pC != NULL){
      SCREENWIDTH=480;
      SCREENHEIGHT=480;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-small"));
    if (pC != NULL){
      SCREENWIDTH/= 2;
      SCREENHEIGHT/= 2;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-320x240"));
    if (pC != NULL){
      SCREENWIDTH=320;
      SCREENHEIGHT=240;
    }
    pC = _tcsstr(MyCommandLine, TEXT("-240x320"));
    if (pC != NULL){
      SCREENWIDTH=240;
      SCREENHEIGHT=320;
    }

#endif
  }
}



bool CheckRectOverlap(RECT rc1, RECT rc2) {
  if(rc1.left >= rc2.right) return(false);
  if(rc1.right <= rc2.left) return(false);
  if(rc1.top >= rc2.bottom) return(false);
  if(rc1.bottom <= rc2.top) return(false);
  return(true);
}


#if !defined(GNAV) || (WINDOWSPC>0)
typedef DWORD (_stdcall *GetIdleTimeProc) (void);
GetIdleTimeProc GetIdleTime;
#endif

int MeasureCPULoad() {
#if (!defined(GNAV) || (WINDOWSPC>0)) && !defined(__MINGW32__)
  static bool init=false;
  if (!init) {
    // get the pointer to the function
    GetIdleTime = (GetIdleTimeProc)
      GetProcAddress(LoadLibrary(_T("coredll.dll")),
		     _T("GetIdleTime"));
    init=true;
  }
  if (!GetIdleTime) return 0;
#endif

#if defined(GNAV) && defined(__MINGW32__)
  // JMW GetIdleTime() not defined?
  return 100;
#else
  static int pi;
  static int PercentIdle;
  static int PercentLoad;
  static bool start=true;
  static DWORD dwStartTick;
  static DWORD dwIdleSt;
  static DWORD dwStopTick;
  static DWORD dwIdleEd;
  if (start) {
    dwStartTick = GetTickCount();
    dwIdleSt = GetIdleTime();
  }
  if (!start) {
    dwStopTick = GetTickCount();
    dwIdleEd = GetIdleTime();
    pi = ((100 * (dwIdleEd - dwIdleSt))/(dwStopTick - dwStartTick));
    PercentIdle = (PercentIdle+pi)/2;
  }
  start = !start;
  PercentLoad = 100-PercentIdle;
  return PercentLoad;
#endif
}


/////////////


int NumberOfFLARMNames = 0;

typedef struct {
  long ID;
  TCHAR Name[21];
} FLARM_Names_t;

#define MAXFLARMNAMES 200

FLARM_Names_t FLARM_Names[MAXFLARMNAMES];

void CloseFLARMDetails() {
  int i;
  for (i=0; i<NumberOfFLARMNames; i++) {
    //    free(FLARM_Names[i]);
  }
  NumberOfFLARMNames = 0;
}

void OpenFLARMDetails() {
  StartupStore(TEXT("OpenFLARMDetails\n"));

  if (NumberOfFLARMNames) {
    CloseFLARMDetails();
  }

  TCHAR filename[MAX_PATH];
  LocalPath(filename,TEXT("xcsoar-flarm.txt"));

  FILE *file = _tfopen(filename, TEXT("rt"));
  if (file == NULL)
    return;

  TCHAR line[READLINE_LENGTH];
  while (ReadStringX(file, READLINE_LENGTH, line)) {
    long id;
    TCHAR Name[MAX_PATH];

    if (_stscanf(line, TEXT("%lx=%s"), &id, Name) == 2) {
      if (AddFlarmLookupItem(id, Name, false) == false)
	{
	  break; // cant add anymore items !
	}
    }
  }

  fclose(file);
}


void SaveFLARMDetails(void)
{
  TCHAR filename[MAX_PATH];
  LocalPath(filename,TEXT("xcsoar-flarm.txt"));

  FILE *file = _tfopen(filename, TEXT("wt"));
  if (file == NULL)
    return;

  TCHAR wsline[READLINE_LENGTH];
  char cline[READLINE_LENGTH];

  for (int z = 0; z < NumberOfFLARMNames; z++)
    {
      wsprintf(wsline, TEXT("%lx=%s\r\n"), FLARM_Names[z].ID,FLARM_Names[z].Name);

#ifdef _UNICODE
      WideCharToMultiByte( CP_ACP, 0, wsline,
			   _tcslen(wsline)+1,
			   cline,
			   READLINE_LENGTH, NULL, NULL);
#else
      strcpy(cline, wsline);
#endif

      fputs(cline, file);
    }

  fclose(file);
}


int LookupSecondaryFLARMId(int id)
{
  for (int i=0; i<NumberOfFLARMNames; i++)
    {
      if (FLARM_Names[i].ID == id)
	{
	  return i;
	}
    }
  return -1;
}

int LookupSecondaryFLARMId(const TCHAR *cn)
{
  for (int i=0; i<NumberOfFLARMNames; i++)
    {
      if (_tcscmp(FLARM_Names[i].Name, cn) == 0)
	{
	  return i;
	}
    }
  return -1;
}


const TCHAR* LookupFLARMDetails(long id) {

  // try to find flarm from userFile
  int index = LookupSecondaryFLARMId(id);
  if (index != -1)
    {
      return FLARM_Names[index].Name;
    }

#ifdef NEWFLARMDB
  // try to find flarm from FLARMNet.org File
  FlarmId* flarmId = file.GetFlarmIdItem(id);
  if (flarmId != NULL)
    {
      return flarmId->cn;
    }
#endif
  return NULL;
}


int LookupFLARMDetails(const TCHAR *cn)
{
  // try to find flarm from userFile
  int index = LookupSecondaryFLARMId(cn);
  if (index != -1)
    {
      return FLARM_Names[index].ID;
    }

#ifdef NEWFLARMDB
  // try to find flarm from FLARMNet.org File
  FlarmId* flarmId = file.GetFlarmIdItem(cn);
  if (flarmId != NULL)
    {
      return flarmId->GetId();
    }
#endif
  return 0;
}


bool AddFlarmLookupItem(int id, const TCHAR *name, bool saveFile)
{
  int index = LookupSecondaryFLARMId(id);

  if (index == -1)
    {
      if (NumberOfFLARMNames < MAXFLARMNAMES - 1)
	{
	  // create new record
	  FLARM_Names[NumberOfFLARMNames].ID = id;
	  _tcsncpy(FLARM_Names[NumberOfFLARMNames].Name, name,20);
	  FLARM_Names[NumberOfFLARMNames].Name[20]=0;
	  NumberOfFLARMNames++;
	  SaveFLARMDetails();
	  return true;
	}
    }
  else
    {
      // modify existing record
      FLARM_Names[index].ID = id;
      _tcsncpy(FLARM_Names[index].Name, name,20);
      FLARM_Names[index].Name[20]=0;
      if (saveFile)
	{
	  SaveFLARMDetails();
	}
      return true;
    }
  return false;
}


long CheckFreeRam(void) {
  MEMORYSTATUS    memInfo;
  // Program memory
  memInfo.dwLength = sizeof(memInfo);
  GlobalMemoryStatus(&memInfo);

  //	   memInfo.dwTotalPhys,
  //	   memInfo.dwAvailPhys,
  //	   memInfo.dwTotalPhys- memInfo.dwAvailPhys);

  return memInfo.dwAvailPhys;
}


#if (WINDOWSPC>0)
#if _DEBUG
_CrtMemState memstate_s1;
#endif
#endif

void MemCheckPoint()
{
#if (WINDOWSPC>0)
#if _DEBUG
  _CrtMemCheckpoint( &memstate_s1 );
#endif
#endif
}


void MemLeakCheck() {
#if (WINDOWSPC>0)
#if _DEBUG
  _CrtMemState memstate_s2, memstate_s3;

   // Store a 2nd memory checkpoint in s2
   _CrtMemCheckpoint( &memstate_s2 );

   if ( _CrtMemDifference( &memstate_s3, &memstate_s1, &memstate_s2 ) ) {
     _CrtMemDumpStatistics( &memstate_s3 );
     _CrtMemDumpAllObjectsSince(&memstate_s1);
   }

  _CrtCheckMemory();
#endif
#endif
}


///////////////

// This is necessary to be called periodically to get rid of
// memory defragmentation, since on pocket pc platforms there is no
// automatic defragmentation.
void MyCompactHeaps() {
#if (WINDOWSPC>0)||(defined(GNAV) && !defined(__MINGW32__))
  HeapCompact(GetProcessHeap(),0);
#else
  typedef DWORD (_stdcall *CompactAllHeapsFn) (void);
  static CompactAllHeapsFn CompactAllHeaps = NULL;
  static bool init=false;
  if (!init) {
    // get the pointer to the function
    CompactAllHeaps = (CompactAllHeapsFn)
      GetProcAddress(LoadLibrary(_T("coredll.dll")),
		     _T("CompactAllHeaps"));
    init=true;
  }
  if (CompactAllHeaps) {
    CompactAllHeaps();
  }
#endif
}


unsigned long FindFreeSpace(const TCHAR *path) {
  // returns number of kb free on destination drive

  ULARGE_INTEGER FreeBytesAvailableToCaller;
  ULARGE_INTEGER TotalNumberOfBytes;
  ULARGE_INTEGER TotalNumberOfFreeBytes;
  if (GetDiskFreeSpaceEx(path,
			 &FreeBytesAvailableToCaller,
			 &TotalNumberOfBytes,
			 &TotalNumberOfFreeBytes)) {
    return FreeBytesAvailableToCaller.LowPart/1024;
  } else {
    return 0;
  }
}


bool MatchesExtension(const TCHAR *filename, const TCHAR* extension) {
  TCHAR *ptr;
  ptr = _tcsstr((TCHAR*)filename, extension);
  if (ptr != filename+_tcslen(filename)-_tcslen(extension)) {
    return false;
  } else {
    return true;
  }
}


#ifndef DISABLEAUDIO
#include "mmsystem.h"
#endif

BOOL PlayResource (const TCHAR* lpName)
{
#ifdef DISABLEAUDIO
  return false;
#else
  BOOL bRtn;
  LPTSTR lpRes;
  HANDLE hResInfo, hRes;

  // TODO code: Modify to allow use of WAV Files and/or Embedded files

  if (_tcsstr(lpName, TEXT(".wav"))) {
    bRtn = sndPlaySound (lpName, SND_ASYNC | SND_NODEFAULT );

  } else {

    // Find the wave resource.
    hResInfo = FindResource (hInst, lpName, TEXT("WAVE"));

    if (hResInfo == NULL)
      return FALSE;

    // Load the wave resource.
    hRes = LoadResource (hInst, (HRSRC)hResInfo);

    if (hRes == NULL)
      return FALSE;

    // Lock the wave resource and play it.
    lpRes = (LPTSTR)LockResource ((HGLOBAL)hRes);

    if (lpRes != NULL)
      {
	bRtn = sndPlaySound (lpRes, SND_MEMORY | SND_ASYNC | SND_NODEFAULT );
      }
    else
      bRtn = 0;
  }
  return bRtn;
#endif
}

void CreateDirectoryIfAbsent(const TCHAR *filename) {
  TCHAR fullname[MAX_PATH];

  LocalPath(fullname, filename);

  DWORD fattr = GetFileAttributes(fullname);

  if ((fattr != 0xFFFFFFFF) &&
      (fattr & FILE_ATTRIBUTE_DIRECTORY)) {
    // directory exists
  } else {
    CreateDirectory(fullname, NULL);
  }

}

//////////

static int interface_timeout;

bool InterfaceTimeoutZero(void) {
  return (interface_timeout==0);
}

void InterfaceTimeoutReset(void) {
  interface_timeout = 0;
}


bool InterfaceTimeoutCheck(void) {
  if (interface_timeout > 60*10) {
    interface_timeout = 0;
    return true;
  } else {
    interface_timeout++;
    return false;
  }
}

bool FileExistsW(const TCHAR *FileName){
  FILE *file = _tfopen(FileName, TEXT("r"));
  if (file == NULL)
    return(FALSE);

  fclose(file);

  return(TRUE);

}

bool FileExistsA(const char *FileName){
  FILE *file = fopen(FileName, "r");
  if (file != NULL) {
    fclose(file);
    return(TRUE);
  }
  return FALSE;
}



bool RotateScreen() {
#if (WINDOWSPC>0)
  return false;
#else
  //
  // Change the orientation of the screen
  //
#ifdef GNAV
  DEVMODE DeviceMode;

  memset(&DeviceMode, 0, sizeof(DeviceMode));
  DeviceMode.dmSize=sizeof(DeviceMode);
  DeviceMode.dmFields = DM_DISPLAYORIENTATION;
  DeviceMode.dmDisplayOrientation = DMDO_90;
  //Put your desired position right here.

  if (DISP_CHANGE_SUCCESSFUL ==
      ChangeDisplaySettingsEx(NULL, &DeviceMode, NULL, CDS_RESET, NULL))
    return true;
  else
    return false;
#else
  return false;
#endif
#endif

}


/*
empty 252 kg
ballast 160kg
max auw 525 kg
Calculated
>LS6 numbers at 33.8 kg/m2
>speed km/h  sink m/s
>80         0.589
>90         0.6
>100        0.658
>110        0.733
>120        0.854
>130        0.984
>140        1.131
>150        1.313
>160        1.510
>170        1.741
>180        1.965
>190        2.209
*/

#ifdef PNA
// VENTA-ADDON MODELTYPE

//
//	Check if the model type is encoded in the executable file name
//
//  GlobalModelName is a global variable, shown during startup and used for printouts only.
//  In order to know what model you are using, GlobalModelType is used.
//
//  This "smartname" facility is used to override the registry/config Model setup to force
//  a model type to be used, just in case. The model types may not follow strictly those in
//  config menu, nor be updated. Does'nt hurt though.
//
void SmartGlobalModelType() {

	GlobalModelType=MODELTYPE_PNA;	// default for ifdef PNA by now!

	if ( GetGlobalModelName() )
	{
		ConvToUpper(GlobalModelName);

		if ( !_tcscmp(GlobalModelName,_T("PNA"))) {
					GlobalModelType=MODELTYPE_PNA_PNA;
					_tcscpy(GlobalModelName,_T("GENERIC") );
		}
		else
			if ( !_tcscmp(GlobalModelName,_T("HP31X")))	{
					GlobalModelType=MODELTYPE_PNA_HP31X;
			}
		else
			if ( !_tcscmp(GlobalModelName,_T("PN6000"))) {
					GlobalModelType=MODELTYPE_PNA_PN6000;
			}
		else
			if ( !_tcscmp(GlobalModelName,_T("MIO"))) {
					GlobalModelType=MODELTYPE_PNA_MIO;
			}
		else
			_tcscpy(GlobalModelName,_T("UNKNOWN") );
	} else
		_tcscpy(GlobalModelName, _T("UNKNOWN") );
}


//
// Retrieve from the registry the previous set model type
// This value is defined in xcsoar.h , example> MODELTYPE_PNA_HP31X
// is equivalent to a value=10201 (defined in the header file)
//
void SetModelType() {

  TCHAR sTmp[100];
  TCHAR szRegistryInfoBoxModel[]= TEXT("AppInfoBoxModel");
  DWORD Temp=0;

  GetFromRegistry(szRegistryInfoBoxModel, &Temp);

  if ( SetModelName(Temp) != true ) {

    _stprintf(sTmp,_T("SetModelType ERROR! ModelName returned invalid value <%d> from Registry!\n"), Temp);
    StartupStore(sTmp);
    GlobalModelType=MODELTYPE_PNA_PNA;

  } else {

    GlobalModelType = Temp;
  }

  _stprintf(sTmp,_T("SetModelType: Name=<%s> Type=%d\n"),GlobalModelName, GlobalModelType);
  StartupStore(sTmp);
}

// Parse a MODELTYPE value and set the equivalent model name.
// If the modeltype is invalid or not yet handled, assume that
// the user changed it in the registry or in the profile, and
// correct the error returning false: this will force a Generic Type.

bool SetModelName(DWORD Temp) {
  switch (Temp) {
  case MODELTYPE_PNA_PNA:
    _tcscpy(GlobalModelName,_T("GENERIC"));
    return true;
    break;
  case MODELTYPE_PNA_HP31X:
    _tcscpy(GlobalModelName,_T("HP31X"));
    return true;
    break;
  case MODELTYPE_PNA_PN6000:
    _tcscpy(GlobalModelName,_T("PN6000"));
    return true;
  case MODELTYPE_PNA_MIO:
    _tcscpy(GlobalModelName,_T("MIO"));
    return true;
  case  MODELTYPE_PNA_MEDION_P5:
    _tcscpy(GlobalModelName,_T("MEDION P5"));
    return true;
  case MODELTYPE_PNA_NOKIA_500:
    _tcscpy(GlobalModelName,_T("NOKIA500"));
    return true;
  default:
    _tcscpy(GlobalModelName,_T("UNKNOWN"));
    return false;
  }

}

#endif


#if defined(PNA) || defined(FIVV)  // VENTA-ADDON gmfpathname & C.

/*
	Paolo Ventafridda 1 feb 08
	Get pathname & c. from GetModuleFilename (gmf)
	In case of problems, always return \ERRORxx\  as path name
	It will be displayed at startup and users will know that
	something is wrong reporting the error code to us.
	Approach not followed: It works but we don't know why
	Approach followed: It doesn't work and we DO know why

	These are temporary solutions to be improved
 */

#define MAXPATHBASENAME MAX_PATH

/*
 * gmfpathname returns the pathname of the current executed program, with leading and trailing slash
 * example:  \sdmmc\   \SD CARD\
 * In case of double slash, it is assumed currently as a single "\" .
 */
TCHAR * gmfpathname ()
{
  static TCHAR gmfpathname_buffer[MAXPATHBASENAME];
  TCHAR  *p;

  if (GetModuleFileName(NULL, gmfpathname_buffer, MAXPATHBASENAME) <= 0) {
//    StartupStore(TEXT("CRITIC- gmfpathname returned null GetModuleFileName\n")); // rob bughunt
    return(_T("\\ERROR_01\\") );
  }
  if (gmfpathname_buffer[0] != '\\' ) {
//   StartupStore(TEXT("CRITIC- gmfpathname starting without a leading backslash\n"));
    return(_T("\\ERROR_02\\"));
  }
  gmfpathname_buffer[MAXPATHBASENAME-1] = '\0';	// truncate for safety

  for (p=gmfpathname_buffer+1; *p != '\0'; p++)
    if ( *p == '\\' ) break;	// search for the very first "\"

  if ( *p == '\0') {
//    StartupStore(TEXT("CRITIC- gmfpathname no backslash found\n"));
    return(_T("\\ERROR_03\\"));
  }
  *++p = '\0';

  return (TCHAR *) gmfpathname_buffer;
}

/*
 * gmfbasename returns the filename of the current executed program, without leading path.
 * Example:  xcsoar.exe
 */
TCHAR * gmfbasename ()
{
  static TCHAR gmfbasename_buffer[MAXPATHBASENAME];
  TCHAR *p, *lp;

  if (GetModuleFileName(NULL, gmfbasename_buffer, MAXPATHBASENAME) <= 0) {
    StartupStore(TEXT("CRITIC- gmfbasename returned null GetModuleFileName\n"));
    return(_T("ERROR_04") );
  }
  if (gmfbasename_buffer[0] != '\\' ) {
    StartupStore(TEXT("CRITIC- gmfbasename starting without a leading backslash\n"));
    return(_T("ERROR_05"));
  }
  for (p=gmfbasename_buffer+1, lp=NULL; *p != '\0'; p++)
    {
      if ( *p == '\\' ) {
	lp=++p;
	continue;
      }
    }
  return  lp;
}

/*
 *	A little hack in the executable filename: if it contains an
 *	underscore, then the following chars up to the .exe is
 *	considered a modelname
 *  Returns 0 if failed, 1 if name found
 */
int GetGlobalModelName ()
{
  TCHAR modelname_buffer[MAXPATHBASENAME];
  TCHAR *p, *lp, *np;

  _tcscpy(GlobalModelName, _T(""));

  if (GetModuleFileName(NULL, modelname_buffer, MAXPATHBASENAME) <= 0) {
    StartupStore(TEXT("CRITIC- GetGlobalFileName returned NULL\n"));
    return 0;
  }
  if (modelname_buffer[0] != '\\' ) {
    StartupStore(TEXT("CRITIC- GetGlobalFileName starting without a leading backslash\n"));
    return 0;
  }
  for (p=modelname_buffer+1, lp=NULL; *p != '\0'; p++)
    {
      if ( *p == '\\' ) {
	lp=++p;
	continue;
      }
    } // assuming \sd\path\xcsoar_pna.exe  we are now at \xcsoar..

  for (p=lp, np=NULL; *p != '\0'; p++)
    {
      if (*p == '_' ) {
	np=++p;
	break;
      }
    } // assuming xcsoar_pna.exe we are now at _pna..

  if ( np == NULL ) {
    return 0;	// VENTA2-bugfix , null deleted
  }

  for ( p=np, lp=NULL; *p != '\0'; p++)
    {
      if (*p == '.' ) {
	lp=p;
	break;
      }
    } // we found the . in pna.exe

  if (lp == NULL) return 0; // VENTA2-bugfix null return
  *lp='\0'; // we cut .exe

  _tcscpy(GlobalModelName, np);

  return 1;  // we say ok

}

#endif   // PNA

/*
 * Convert to uppercase a TCHAR array
 */
void ConvToUpper( TCHAR *str )
{
	if ( str )
	{
		for ( ; *str; ++str )
		*str = towupper(*str);

	}

	return ;
}

#ifdef FIVV
BOOL DelRegistryKey(const TCHAR *szDelKey)
{
   HKEY tKey;
   RegOpenKeyEx(HKEY_CURRENT_USER, _T(REGKEYNAME),0,0,&tKey);
   if ( RegDeleteValue(tKey, szDelKey) != ERROR_SUCCESS ) {
	return false;
   }
   RegCloseKey(tKey);
   return true;
}
#endif

#ifdef PNA
void CleanRegistry()
{
   HKEY tKey;
   RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryKey ,0,0,&tKey);

	RegDeleteValue(tKey,_T("CDIWindowFont"));
	RegDeleteValue(tKey,_T("InfoWindowFont"));
	RegDeleteValue(tKey,_T("MapLabelFont"));
	RegDeleteValue(tKey,_T("MapWindowBoldFont"));
	RegDeleteValue(tKey,_T("MapWindowFont"));
	RegDeleteValue(tKey,_T("StatisticsFont"));
	RegDeleteValue(tKey,_T("TitleSmallWindowFont"));
	RegDeleteValue(tKey,_T("TitleWindowFont"));
	RegDeleteValue(tKey,_T("BugsBallastFont"));
	RegDeleteValue(tKey,_T("TeamCodeFont"));

   RegCloseKey(tKey);
}
#endif

#ifdef PNA
/* Paolo Ventafridda Apr 23th 2009 VENTA4
 * SetBacklight for PNA devices. There is no standard way of managing backlight on CE,
 * and every device may have different value names and settings. Microsoft did not set
 * a standard and thus we need a custom solution for each device.
 * But the approach is always the same: change a value and call an event.
 * We do this in XCSoar.cpp at the beginning, no need to make these settings configurable:
 * max brightness and no timeout if on power is the rule. Otherwise, do it manually..
 */
bool SetBacklight() // VENTA4
{
  HKEY    hKey;
  DWORD   Disp=0;
  HRESULT hRes;
  bool doevent=false;

  if (EnableAutoBacklight == false ) return false;

  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\Backlight"), 0,  0, &hKey);
  if (hRes != ERROR_SUCCESS) return false;

  switch (GlobalModelType)
  {
	case MODELTYPE_PNA_HP31X:

		Disp=20; // max backlight
		// currently we ignore hres, if registry entries are spoiled out user is already in deep troubles
		hRes = RegSetValueEx(hKey, _T("BackLightCurrentACLevel"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		hRes = RegSetValueEx(hKey, _T("BackLightCurrentBatteryLevel"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		hRes = RegSetValueEx(hKey, _T("TotalLevels"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=0;
		hRes = RegSetValueEx(hKey, _T("UseExt"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		RegDeleteValue(hKey,_T("ACTimeout"));
		doevent=true;
		break;

	default:
		doevent=false;
		break;
  }

  RegCloseKey(hKey); if (doevent==false) return false;

  HANDLE BLEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("BacklightChangeEvent"));
  if ( SetEvent(BLEvent) == 0) doevent=false;
  	else CloseHandle(BLEvent);
  return doevent;
}

bool SetSoundVolume() // VENTA4
{

  if (EnableAutoSoundVolume == false ) return false;

/*
 * This does not work, dunno why
 *
  HKEY    hKey;
  DWORD   Disp=0;
  HRESULT hRes;

  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\Volume"), 0,  0, &hKey);
  if (hRes != ERROR_SUCCESS) return false;
  switch (GlobalModelType)
  {
	case MODELTYPE_PNA_HP31X:
		Disp=0xFFFFFFFF; // max volume
		hRes = RegSetValueEx(hKey, _T("Volume"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=65538;
		hRes = RegSetValueEx(hKey, _T("Screen"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=0;
		hRes = RegSetValueEx(hKey, _T("Key"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		Disp=7;
		hRes = RegSetValueEx(hKey, _T("Mute"),0,REG_DWORD, (LPBYTE)&Disp, sizeof(DWORD));
		SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0xF2, 0);
	        RegCloseKey(hKey);
		break;

	default:
		break;
  }
 */

  // should we enter critical section ?  probably...
  waveOutSetVolume(0, 0xffff); // this is working for all platforms

  return true;
}


#endif

#if defined(FIVV) || defined(PNA)
// VENTA2-ADDON fonts install
/*
 * Get the localpath, enter XCSoarData/Config, see if there are fonts to copy,
 * check that they have not already been copied in \Windows\Fonts,
 * and eventually copy everything in place.
 *
 * Returns: 0 if OK .
 * 1 - n other errors not really needed to handle. See below
 *
 * These are currently fonts used by PDA:
 *
	DejaVuSansCondensed2.ttf
	DejaVuSansCondensed-Bold2.ttf
	DejaVuSansCondensed-BoldOblique2.ttf
	DejaVuSansCondensed-Oblique2.ttf
 *
 *
 */
short InstallFonts() {

TCHAR srcdir[MAX_PATH];
TCHAR dstdir[MAX_PATH];
TCHAR srcfile[MAX_PATH];
TCHAR dstfile[MAX_PATH];

_stprintf(srcdir,TEXT("%s%S\\Fonts"),gmfpathname(), XCSDATADIR );
_stprintf(dstdir,TEXT("\\Windows\\Fonts"),gmfpathname() );


if (  GetFileAttributes(srcdir) != FILE_ATTRIBUTE_DIRECTORY) return 1;
if (  GetFileAttributes(dstdir) != FILE_ATTRIBUTE_DIRECTORY) return 2;

_stprintf(srcfile,TEXT("%s\\DejaVuSansCondensed2.ttf"),srcdir);
_stprintf(dstfile,TEXT("%s\\DejaVuSansCondensed2.ttf"),dstdir);
//if (  GetFileAttributes(srcfile) != FILE_ATTRIBUTE_NORMAL) return 3;
if (  GetFileAttributes(dstfile) != 0xffffffff ) return 4;
if ( !CopyFile(srcfile, dstfile, TRUE)) return 5;

// From now on we attempt to copy without overwriting
_stprintf(srcfile,TEXT("%s\\DejaVuSansCondensed-Bold2.ttf"),srcdir);
_stprintf(dstfile,TEXT("%s\\DejaVuSansCondensed-Bold2.ttf"),dstdir);
CopyFile(srcfile,dstfile,TRUE);

_stprintf(srcfile,TEXT("%s\\DejaVuSansCondensed-BoldOblique2.ttf"),srcdir);
_stprintf(dstfile,TEXT("%s\\DejaVuSansCondensed-BoldOblique2.ttf"),dstdir);
CopyFile(srcfile,dstfile,TRUE);

_stprintf(srcfile,TEXT("%s\\DejaVuSansCondensed-Oblique2.ttf"),srcdir);
_stprintf(dstfile,TEXT("%s\\DejaVuSansCondensed-Oblique2.ttf"),dstdir);
CopyFile(srcfile,dstfile,TRUE);


return 0;

}

/*
 * Check that XCSoarData exist where it should be
 * Return false if error, true if Ok
 */
bool CheckDataDir() {
	TCHAR srcdir[MAX_PATH];

	_stprintf(srcdir,TEXT("%s%S"),gmfpathname(), XCSDATADIR );
	if (  GetFileAttributes(srcdir) != FILE_ATTRIBUTE_DIRECTORY) return false;
	return true;
}

/*
 * Check for xcsoar-registry.prf  existance
 * Should really check if geometry has changed.. in 5.2.3!
 * Currently we disable it for HP31X which is the only PNA with different settings
 * for different geometries
 * 5.2.3 BOOL changed to bool
 * TODO: VENTA4 now that Rob fonts is used, should return false all the way
 */
bool CheckRegistryProfile() {
	TCHAR srcpath[MAX_PATH];
	if ( GlobalModelType == MODELTYPE_PNA_HP31X ) return false;
	_stprintf(srcpath,TEXT("%s%S\\%S"),gmfpathname(), XCSDATADIR , XCSPROFILE);
	if (  GetFileAttributes(srcpath) == 0xffffffff) return false;
	return true;
}
#endif
