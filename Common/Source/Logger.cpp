/*
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
#include "Logger.h"
#include "externs.h"
#include "Port.h"
#include <windows.h>
#include <tchar.h>
#include "Utils.h"
#include "device.h"

/*
problems with current IGC:

must have unique serial number, make and type, version number

must mark where declaration is changed in-flight

header: HF TAKEOFF

Pilot Event Marker: PEV

should be on for a while prior to takeoff and off for a while after
landing, to mark the ground level.

add C lines at waypoint events: START, TURN, TURN AREA, FINISH

C lines are causing problems when blank, as reported by Deniz

*/

/*

HFDTE141203  <- should be UTC, same as time in filename
HFFXA100
HFPLTPILOT:JOHN WHARINGTON
HFGTYGLIDERTYPE:LS 3
HFGIDGLIDERID:VH-WUE
HFDTM100GPSDATUM:WGS84
HFRFWFIRMWAREVERSION:3.6
HFRHWHARDWAREVERSION:3.4
HFFTYFR TYPE:GARRECHT INGENIEURGESELLSCHAFT,VOLKSLOGGER 1.0
HFCIDCOMPETITIONID:WUE
HFCCLCOMPETITIONCLASS:FAI
HFCIDCOMPETITIONID:WUE
HFCCLCOMPETITIONCLASS:15M
*/




static TCHAR szLoggerFileName[MAX_PATH];

int EW_count = 0;


void LogPoint(double Latitude, double Longitude, double Altitude)
{
  HANDLE hFile;// = INVALID_HANDLE_VALUE;
  DWORD dwBytesRead;

  SYSTEMTIME st;
  char szBRecord[500];

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;


  DegLat = (int)Latitude;
  MinLat = Latitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)Longitude ;
  MinLon = Longitude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  GetLocalTime(&st);

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
		     NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  sprintf(szBRecord,"B%02d%02d%02d%02d%05.0f%c%03d%05.0f%cA%05d%05d0AA\r\n",
	  st.wHour, st.wMinute, st.wSecond,
	  DegLat, MinLat, NoS, DegLon, MinLon, EoW,
	  (int)Altitude,(int)Altitude);

  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, szBRecord, strlen(szBRecord), &dwBytesRead,
	    (OVERLAPPED *)NULL);

  CloseHandle(hFile);
}


void StartLogger(TCHAR *strAssetNumber)
{
  SYSTEMTIME st;
  HANDLE hFile;
  int i;

  GetLocalTime(&st);

  for(i=1;i<99;i++)
    {
      // 2003-12-31-XXX-987-01.IGC
      // long filename form of IGC file.
      // XXX represents manufacturer code
       wsprintf(szLoggerFileName,TEXT("%s%04d-%02d-%02d-XXX-%c%c%c-%02d.IGC"),
		   LocalPath(),
		st.wYear,
		st.wMonth,
		st.wDay,
		strAssetNumber[0],
		strAssetNumber[1],
		strAssetNumber[2],
		i);

      hFile = CreateFile(szLoggerFileName, GENERIC_WRITE,
			 FILE_SHARE_WRITE, NULL, CREATE_NEW,
			 FILE_ATTRIBUTE_NORMAL, 0);
      if(hFile!=INVALID_HANDLE_VALUE )
	{
	  CloseHandle(hFile);
	  return;
	}
    }
}


extern TCHAR szRegistryPilotName[];
extern TCHAR szRegistryAircraftType[];
extern TCHAR szRegistryAircraftRego[];


void LoggerHeader(void)
{

  char temp[100];
  HANDLE hFile;
  DWORD dwBytesRead;
  TCHAR PilotName[100];
  TCHAR AircraftType[100];
  TCHAR AircraftRego[100];

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE,
		     FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
		     FILE_ATTRIBUTE_NORMAL, 0);

  SetFilePointer(hFile, 0, NULL, FILE_END);

  // Flight recorder ID number MUST go first..
  sprintf(temp,
	  "AXXX%C%C%C\r\n",
	  strAssetNumber[0],
	  strAssetNumber[1],
	  strAssetNumber[2]);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  SYSTEMTIME st;
  GetLocalTime(&st);

  sprintf(temp,"HFDTE%02d%02d%02d\r\n",
	  st.wDay,
	  st.wMonth,
	  st.wYear % 100);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  GetRegistryString(szRegistryPilotName, PilotName, 100);
  sprintf(temp,"HFPLTPILOT:%S\r\n", PilotName);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  GetRegistryString(szRegistryAircraftType, AircraftType, 100);
  sprintf(temp,"HFGTYGLIDERTYPE:%S\r\n", AircraftType);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  GetRegistryString(szRegistryAircraftRego, AircraftRego, 100);
  sprintf(temp,"HFGIDGLIDERID:%S\r\n", AircraftRego);
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  sprintf(temp,"HFFTYFR TYPE:XCSOAR,XCSOAR %S\r\n", XCSoar_Version);
  CloseHandle(hFile);

}


void StartDeclaration(int ntp)
{
  // JMW TODO: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ETAKEOFF (not defined)\r\n";
  HANDLE hFile;
  DWORD dwBytesRead;
  char temp[100];

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  SetFilePointer(hFile, 0, NULL, FILE_END);

  // JMW added task start declaration line

  SYSTEMTIME st;

  GetLocalTime(&st);

  // IGC GNSS specification 3.6.1
  sprintf(temp,
    "C%02d%02d%02d%02d%02d%02d%02d%02d%02d0000%02d (not defined)\r\n",
	  // DD  MM  YY  HH  MM  SS  DD  MM  YYIIII  TT
	  // JMW TODO these should be UTC time and date!
	   st.wDay,
	   st.wMonth,
	   st.wYear % 100,
	   st.wHour,
	   st.wMinute,
	   st.wSecond,

	  // these should be local date
	   st.wDay,
	   st.wMonth,
	   st.wYear % 100,
	   ntp);

  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  // takeoff line
  // IGC GNSS specification 3.6.3
  WriteFile(hFile, start, strlen(start), &dwBytesRead, (OVERLAPPED *)NULL);

  CloseHandle(hFile);
}


void EndDeclaration(void)
{
  // JMW TODO: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ELANDING (not defined)\r\n";
  HANDLE hFile;
  DWORD dwBytesRead;

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, start, strlen(start), &dwBytesRead,(OVERLAPPED *)NULL);

  CloseHandle(hFile);
}

void AddDeclaration(double Latitude, double Longitude, TCHAR *ID)
{
  DWORD dwBytesRead;
  HANDLE hFile;

  SYSTEMTIME st;
  char szCRecord[500];

  char IDString[100];
  int i;

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  for(i=0;i<(int)_tcslen(ID);i++)
    {
      IDString[i] = (char)ID[i];
    }
  IDString[i] = '\0';

  DegLat = (int)Latitude;
  MinLat = Latitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)Longitude ;
  MinLon = Longitude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  GetLocalTime(&st);

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  sprintf(szCRecord,"C%02d%05.0f%c%03d%05.0f%c%s\r\n", DegLat, MinLat, NoS, DegLon, MinLon, EoW, IDString);



  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, szCRecord, strlen(szCRecord), &dwBytesRead, (OVERLAPPED *)NULL);

  CloseHandle(hFile);
}


// JMW TODO: make this thread-safe, since it could happen in the middle
// of the calculations doing LogPoint or something else!

void LoggerNote(TCHAR *text) {
  if (LoggerActive) {
    HANDLE hFile;// = INVALID_HANDLE_VALUE;
    DWORD dwBytesRead;

    char fulltext[500];
    hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
		       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    sprintf(fulltext, "LPLT%S\r\n", text);
    SetFilePointer(hFile, 0, NULL, FILE_END);
    WriteFile(hFile, fulltext, strlen(fulltext), &dwBytesRead,
	      (OVERLAPPED *)NULL);

    CloseHandle(hFile);

  }
}



void DoLogger(TCHAR *strAssetNumber)
{
  TCHAR TaskMessage[1024];
  int i;

  if(LoggerActive)
    {
      if(MessageBoxX(hWndMapWindow,
		     gettext(TEXT("Stop Logger?")),
		     gettext(TEXT("Stop Logger?")),
		     MB_YESNO|MB_ICONQUESTION) == IDYES)
	{
	  LoggerActive = false;
	}
    }
  else
    {
      _tcscpy(TaskMessage,gettext(TEXT("Start Logger With Declaration?")));
      _tcscat(TaskMessage,TEXT("\r\n"));
      for(i=0;i<MAXTASKPOINTS;i++)
	{
	  if(Task[i].Index == -1)
	    {
	      if(i==0)
		{
		  _tcscat(TaskMessage,gettext(TEXT("None")));
		}
	      break;
	    }
	  _tcscat(TaskMessage,WayPointList[ Task[i].Index ].Name);
	  _tcscat(TaskMessage,TEXT("\r\n"));
	}

      if(MessageBoxX(hWndMapWindow,
		     TaskMessage,
		     gettext(TEXT("Start Logger?")),
		     MB_YESNO|MB_ICONQUESTION) == IDYES)
	{
	  LoggerActive = true;
	  StartLogger(strAssetNumber);
	  LoggerHeader();
	  int ntp = 0;

	  // first count the number of turnpoints
	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) break;
	      ntp++;
	    }
	  StartDeclaration(ntp);

	  for(i=0;i<MAXTASKPOINTS;i++)
	    {
	      if(Task[i].Index == -1) break;
	      AddDeclaration(WayPointList[Task[i].Index].Latitude , WayPointList[Task[i].Index].Longitude  , WayPointList[Task[i].Index].Name );
	    }
	  EndDeclaration();
	}
    }
}


bool DeclaredToDevice = false;

void LoggerDeviceDeclare() {
  int i;
  WCHAR PilotName[64];
  WCHAR AircraftType[32];
  WCHAR AircraftRego[32];
  bool foundone = false;

  GetRegistryString(szRegistryPilotName, PilotName, 64);
  GetRegistryString(szRegistryAircraftType, AircraftType, 32);
  GetRegistryString(szRegistryAircraftRego, AircraftRego, 32);

  if (devIsLogger(devA())){
    foundone = true;
    if(MessageBoxX(hWndMapWindow,
		   gettext(TEXT("Declare Task?")),
		   devA()->Name,
		   MB_YESNO| MB_ICONQUESTION) == IDYES)
      {

	devDeclBegin(devA(), PilotName, AircraftType, AircraftRego);

	for(i=0;i<MAXTASKPOINTS;i++)
	  {
	    if(Task[i].Index == -1) break;
	    devDeclAddWayPoint(devA(), &WayPointList[Task[i].Index]);
	  }
	if (devDeclEnd(devA())) {
	  MessageBoxX(hWndMapWindow,
		      gettext(TEXT("Task Declared!")),
		      devA()->Name, MB_OK| MB_ICONINFORMATION);
	  DeclaredToDevice = true;
	} else
	  MessageBoxX(hWndMapWindow,
		      gettext(TEXT("Error occured,\r\nTask NOT Declared!")),
		      devA()->Name, MB_OK| MB_ICONERROR);

      }
  }

  if (devIsLogger(devB())){
    foundone = true;

    if(MessageBoxX(hWndMapWindow,
		   gettext(TEXT("Declare Task?")),
		   devB()->Name, MB_YESNO| MB_ICONQUESTION) == IDYES){

      devDeclBegin(devB(), PilotName, AircraftType, AircraftRego);
      for(i=0;i<MAXTASKPOINTS;i++)
	{
	  if(Task[i].Index == -1) break;
	  devDeclAddWayPoint(devB(), &WayPointList[Task[i].Index]);
	}
      if (devDeclEnd(devB())) {
	MessageBoxX(hWndMapWindow, gettext(TEXT("Task Declared!")),
			    devB()->Name, MB_OK| MB_ICONINFORMATION);
	  DeclaredToDevice = true;
      } else
	MessageBoxX(hWndMapWindow,
		    gettext(TEXT("Error occured,\r\nTask NOT Declared!")),
		    devB()->Name, MB_OK| MB_ICONERROR);

    }
  }
  if (!foundone) {
    MessageBoxX(hWndMapWindow, gettext(TEXT("No logger connected")),
		devB()->Name, MB_OK| MB_ICONINFORMATION);
    DeclaredToDevice = true; // testing only
  }
}


bool CheckDeclaration(void) {
  if (!DeclaredToDevice) {
    return true;
  } else {
    if(MessageBoxX(hWndMapWindow,
		   gettext(TEXT("OK to invalidate declaration?")),
		   gettext(TEXT("Task declared")),
		   MB_YESNO| MB_ICONQUESTION) == IDYES){
      DeclaredToDevice = false;
      return true;
    } else {
      return false;
    }
  }
}
