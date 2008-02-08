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
#include "InputEvents.h"
#include "Parser.h"

extern NMEA_INFO GPS_INFO;
bool DisableAutoLogger = false;

TCHAR NumToIGCChar(int n) {
  if (n<10) {
    return _T('1') + (n-1);
  } else {
    return _T('A') + (n-10);
  }
}


int IGCCharToNum(TCHAR c) {
  if ((c >= _T('1')) && (c<= _T('9'))) {
    return c- _T('1') + 1;
  } else if ((c >= _T('A')) && (c<= _T('Z'))) {
    return c- _T('A') + 10;
  } else {
    return 0; // Error!
  }
}


/*
problems with current IGC:

must have unique serial number, make and type, version number

must mark where declaration is changed in-flight

header: HF TAKEOFF

Pilot Event Marker: PEV

should be on for a while prior to takeoff and off for a while after
landing, to mark the ground level.

PEVs should also activate fast-fix (for how long?)

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


static TCHAR szLoggerFileName[MAX_PATH] = TEXT("\0");
static TCHAR szFLoggerFileName[MAX_PATH] = TEXT("\0");

int EW_count = 0;
int NumLoggerBuffered = 0;

#define MAX_LOGGER_BUFFER 60

typedef struct LoggerBuffer {
  double Latitude;
  double Longitude;
  double Altitude;
  double BaroAltitude;
  short Day;
  short Month;
  short Year;
  short Hour;
  short Minute;
  short Second;
} LoggerBuffer_T;

LoggerBuffer_T FirstPoint;
LoggerBuffer_T LoggerBuffer[MAX_LOGGER_BUFFER];


void StopLogger(void) {
  if (LoggerActive) {
    LoggerActive = false;
    if (LoggerClearFreeSpace()) {
      MoveFile(szLoggerFileName, szFLoggerFileName);
    }
    NumLoggerBuffered = 0;
  }
}


void LogPointToBuffer(double Latitude, double Longitude, double Altitude,
                      double BaroAltitude, short Hour, short Minute, short Second) {

  if (NumLoggerBuffered== MAX_LOGGER_BUFFER) {
    for (int i= 0; i< NumLoggerBuffered-1; i++) {
      LoggerBuffer[i]= LoggerBuffer[i+1];
    }
  } else {
    NumLoggerBuffered++;
  }
  LoggerBuffer[NumLoggerBuffered-1].Latitude = Latitude;
  LoggerBuffer[NumLoggerBuffered-1].Longitude = Longitude;
  LoggerBuffer[NumLoggerBuffered-1].Altitude = Altitude;
  LoggerBuffer[NumLoggerBuffered-1].BaroAltitude = BaroAltitude;
  LoggerBuffer[NumLoggerBuffered-1].Hour = Hour;
  LoggerBuffer[NumLoggerBuffered-1].Minute = Minute;
  LoggerBuffer[NumLoggerBuffered-1].Second = Second;
  LoggerBuffer[NumLoggerBuffered-1].Year = GPS_INFO.Year;
  LoggerBuffer[NumLoggerBuffered-1].Month = GPS_INFO.Month;
  LoggerBuffer[NumLoggerBuffered-1].Day = GPS_INFO.Day;

  // This is the first point that will be output to file.
  // Declaration must happen before this, so must save this time.
  FirstPoint = LoggerBuffer[0];
}


void LogPointToFile(double Latitude, double Longitude, double Altitude,
                    double BaroAltitude, short Hour, short Minute, short Second)
{
  HANDLE hFile;// = INVALID_HANDLE_VALUE;
  DWORD dwBytesRead;
  char szBRecord[500];

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  if (Altitude<0) return;
  if (BaroAltitude<0) return;

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

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
		     NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  sprintf(szBRecord,"B%02d%02d%02d%02d%05.0f%c%03d%05.0f%cA%05d%05d\r\n",
          Hour, Minute, Second,
          DegLat, MinLat, NoS, DegLon, MinLon, EoW,
          (int)Altitude,(int)BaroAltitude);

  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, szBRecord, strlen(szBRecord), &dwBytesRead,
	    (OVERLAPPED *)NULL);
  FlushFileBuffers(hFile);

  CloseHandle(hFile);
}


void LogPoint(double Latitude, double Longitude, double Altitude,
              double BaroAltitude) {
  if (!LoggerActive) {
    if (!GPS_INFO.NAVWarning) {
      LogPointToBuffer(Latitude, Longitude, Altitude, BaroAltitude,
                       GPS_INFO.Hour, GPS_INFO.Minute, GPS_INFO.Second);
    }
  } else if (NumLoggerBuffered) {
    for (int i=0; i<NumLoggerBuffered; i++) {
      LogPointToFile(LoggerBuffer[i].Latitude,
                     LoggerBuffer[i].Longitude,
                     LoggerBuffer[i].Altitude,
                     LoggerBuffer[i].BaroAltitude,
                     LoggerBuffer[i].Hour,
                     LoggerBuffer[i].Minute,
                     LoggerBuffer[i].Second);
    }
    NumLoggerBuffered = 0;
  }
  if (LoggerActive) {
    LogPointToFile(Latitude, Longitude, Altitude, BaroAltitude,
                   GPS_INFO.Hour, GPS_INFO.Minute, GPS_INFO.Second);
  }
}


void StartLogger(TCHAR *strAssetNumber)
{
  HANDLE hFile;
  int i;
  TCHAR path[MAX_PATH];
#ifdef GNAV
  LocalPath(path,TEXT("logs"));
#else
  LocalPath(path);
#endif

  wsprintf(szLoggerFileName,
           TEXT("\\tmp.IGC"));
  DeleteFile(szLoggerFileName);

  for(i=1;i<99;i++)
    {
      // 2003-12-31-XXX-987-01.IGC
      // long filename form of IGC file.
      // XXX represents manufacturer code

      if (!LoggerShortName) {
        // Long file name
        wsprintf(szFLoggerFileName,
                 TEXT("%s\\%04d-%02d-%02d-XXX-%c%c%c-%02d.IGC"),
                 path,
                 GPS_INFO.Year,
                 GPS_INFO.Month,
                 GPS_INFO.Day,
                 strAssetNumber[0],
                 strAssetNumber[1],
                 strAssetNumber[2],
                 i);
      } else {
        // Short file name
        TCHAR cyear, cmonth, cday, cflight;
        cyear = NumToIGCChar((int)GPS_INFO.Year % 10);
        cmonth = NumToIGCChar(GPS_INFO.Month);
        cday = NumToIGCChar(GPS_INFO.Day);
        cflight = NumToIGCChar(i);
        wsprintf(szFLoggerFileName,
                 TEXT("%s\\%c%c%cX%c%c%c%c.IGC"),
                 path,
                 cyear,
                 cmonth,
                 cday,
                 strAssetNumber[0],
                 strAssetNumber[1],
                 strAssetNumber[2],
                 cflight);
      }

      hFile = CreateFile(szFLoggerFileName, GENERIC_WRITE,
			 FILE_SHARE_WRITE, NULL, CREATE_NEW,
			 FILE_ATTRIBUTE_NORMAL, 0);
      if(hFile!=INVALID_HANDLE_VALUE )
	{
          // file already exists
	  CloseHandle(hFile);
          DeleteFile(szFLoggerFileName);
	  return;
	}
    }
}


extern TCHAR szRegistryPilotName[];
extern TCHAR szRegistryAircraftType[];
extern TCHAR szRegistryAircraftRego[];


void LoggerHeader(void)
{
  char datum[]= "HFDTM100Datum: WGS-84\r\n";
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

  sprintf(temp,"HFDTE%02d%02d%02d\r\n",
	  GPS_INFO.Day,
	  GPS_INFO.Month,
	  GPS_INFO.Year % 100);
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
  WriteFile(hFile, temp, strlen(temp), &dwBytesRead,(OVERLAPPED *)NULL);

  WriteFile(hFile, datum, strlen(datum), &dwBytesRead,(OVERLAPPED *)NULL);

  FlushFileBuffers(hFile);
  CloseHandle(hFile);

}


void StartDeclaration(int ntp)
{
  // JMW TODO: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ETAKEOFF\r\n";
  HANDLE hFile;
  DWORD dwBytesRead;
  char temp[100];

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE,
                     FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, 0);

  SetFilePointer(hFile, 0, NULL, FILE_END);

  if (NumLoggerBuffered==0) {
     FirstPoint.Year = GPS_INFO.Year;
     FirstPoint.Month = GPS_INFO.Month;
     FirstPoint.Day = GPS_INFO.Day;
     FirstPoint.Hour = GPS_INFO.Hour;
     FirstPoint.Minute = GPS_INFO.Minute;
     FirstPoint.Second = GPS_INFO.Second;
  }

  // JMW added task start declaration line

  // LGCSTKF013945TAKEOFF DETECTED

  // IGC GNSS specification 3.6.1
  sprintf(temp,
	  "C%02d%02d%02d%02d%02d%02d0000000000%02d\r\n",
	  // DD  MM  YY  HH  MM  SS  DD  MM  YY IIII TT
	  FirstPoint.Day,
	  FirstPoint.Month,
	  FirstPoint.Year % 100,
	  FirstPoint.Hour,
	  FirstPoint.Minute,
	  FirstPoint.Second,
	  ntp-2);

  WriteFile(hFile, temp, strlen(temp), &dwBytesRead, (OVERLAPPED *)NULL);

  // takeoff line
  // IGC GNSS specification 3.6.3
  WriteFile(hFile, start, strlen(start), &dwBytesRead, (OVERLAPPED *)NULL);

  FlushFileBuffers(hFile);

  CloseHandle(hFile);
}


void EndDeclaration(void)
{
  // JMW TODO: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ELANDING\r\n";
  HANDLE hFile;
  DWORD dwBytesRead=0;

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE,
		     FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, 0);

  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, start, strlen(start), &dwBytesRead,(OVERLAPPED *)NULL);
  FlushFileBuffers(hFile);

  CloseHandle(hFile);
}

void AddDeclaration(double Latitude, double Longitude, TCHAR *ID)
{
  DWORD dwBytesRead;
  HANDLE hFile;
  char szCRecord[500];

  char IDString[MAX_PATH];
  int i;

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  TCHAR tmpstring[MAX_PATH];
  _tcscpy(tmpstring, ID);
  _tcsupr(tmpstring);
  for(i=0;i<(int)_tcslen(tmpstring);i++)
    {
      IDString[i] = (char)tmpstring[i];
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

  hFile = CreateFile(szLoggerFileName,
		     GENERIC_WRITE, FILE_SHARE_WRITE,
		     NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  sprintf(szCRecord,"C%02d%05.0f%c%03d%05.0f%c%s\r\n",
	  DegLat, MinLat, NoS, DegLon, MinLon, EoW, IDString);

  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, szCRecord, strlen(szCRecord), &dwBytesRead, (OVERLAPPED *)NULL);
  FlushFileBuffers(hFile);

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
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

  }
}




void DoLogger(TCHAR *strAssetNumber)
{
  TCHAR TaskMessage[1024];
  int i;

  if (ReplayLogger::IsEnabled()) {
    if (LoggerActive)
      guiStopLogger(true);
    return;
  }

  if(LoggerActive)
    {
      if(MessageBoxX(hWndMapWindow,
		     gettext(TEXT("Stop Logger?")),
		     gettext(TEXT("Stop Logger?")),
		     MB_YESNO|MB_ICONQUESTION) == IDYES)
	{
          StopLogger();
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

	  if (LoggerClearFreeSpace()) {
	    LoggerActive = true;

	    StartLogger(strAssetNumber);
	    LoggerHeader();
	    int ntp = 0;

            RefreshTask();

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
		AddDeclaration(WayPointList[Task[i].Index].Latitude,
			       WayPointList[Task[i].Index].Longitude,
			       WayPointList[Task[i].Index].Name);
	      }
	    EndDeclaration();
	  } else {

	    MessageBoxX(hWndMapWindow,
			gettext(TEXT("Logger inactive, insufficient storage!")),
			gettext(TEXT("Logger Error")), MB_OK| MB_ICONERROR);
	  }
	}
    }
}


bool DeclaredToDevice = false;


void LoggerDeviceDeclare() {
  int i;
  WCHAR PilotName[64];
  WCHAR AircraftType[32];
  WCHAR AircraftRego[32];
  bool found_logger = false;

  GetRegistryString(szRegistryPilotName, PilotName, 64);
  GetRegistryString(szRegistryAircraftType, AircraftType, 32);
  GetRegistryString(szRegistryAircraftRego, AircraftRego, 32);

  if (devIsLogger(devA())){
    found_logger = true;
    DeclaredToDevice = false;

    if(MessageBoxX(hWndMapWindow,
		   gettext(TEXT("Declare Task?")),
		   devA()->Name,
		   MB_YESNO| MB_ICONQUESTION) == IDYES)
      {

        // Must lock comms for entire declaration
        LockComm();

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
	} else {
	  MessageBoxX(hWndMapWindow,
		      gettext(TEXT("Error occured,\r\nTask NOT Declared!")),
		      devA()->Name, MB_OK| MB_ICONERROR);
          DeclaredToDevice = false;
        }

        UnlockComm();
      }
  }

  if (devIsLogger(devB())){
    if (!found_logger) {
      DeclaredToDevice = false;
    }
    found_logger = true;

    if(MessageBoxX(hWndMapWindow,
		   gettext(TEXT("Declare Task?")),
		   devB()->Name, MB_YESNO| MB_ICONQUESTION) == IDYES){

      // Must lock comms for entire declaration
      LockComm();

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
      } else {
	MessageBoxX(hWndMapWindow,
		    gettext(TEXT("Error occured,\r\nTask NOT Declared!")),
		    devB()->Name, MB_OK| MB_ICONERROR);
        DeclaredToDevice = false;
      }

      UnlockComm();

    }
  }

  if (!found_logger) {
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


/////////////////////////


bool ReplayLogger::ReadLine(TCHAR *buffer) {
  static FILE *fp  = NULL;
  if (!buffer) {
    if (fp) {
      fclose(fp);
      fp= NULL;
    }
    return false;
  }
  if (!fp) {
    if (_tcslen(FileName)>0) {
      fp = _tfopen(FileName, TEXT("r"));
    }
  }
  if (fp==NULL)
    return false;

  if (!fgetws(buffer, 200, fp)) {
	_tcscat(buffer,TEXT("\0"));
    return false;
  }
  return true;
}


bool ReplayLogger::ScanBuffer(TCHAR *buffer, double *Time, double *Latitude,
			      double *Longitude, double *Altitude)
{
  int DegLat, DegLon;
  int MinLat, MinLon;
  char NoS, EoW;
  int iAltitude;
  int Hour=0;
  int Minute=0;
  int Second=0;

  int lfound=0;
  int found=0;

  if ((lfound =
       swscanf(buffer,
	       TEXT("B%02d%02d%02d%02d%05d%c%03d%05d%cA%05d%05dd"),
	       &Hour, &Minute, &Second,
	       &DegLat, &MinLat, &NoS, &DegLon, &MinLon,
	       &EoW, &iAltitude, &iAltitude
	       )) != EOF) {

    if (lfound==11) {
      *Latitude = DegLat+MinLat/60000.0;
      if (NoS=='S') {
	*Latitude *= -1;
      }

      *Longitude = DegLon+MinLon/60000.0;
      if (EoW=='W') {
	*Longitude *= -1;
      }
      *Altitude = iAltitude;
      *Time = Hour*3600+Minute*60+Second;
    }
  }

  TCHAR event[200];
  TCHAR misc[200];

  found = _stscanf(buffer,
		   TEXT("LPLT event=%[^ ] %[A-Za-z0-9 \\/().,]"),
		   event,misc);
  if (found>0) {
    pt2Event fevent = InputEvents::findEvent(event);
    if (fevent) {
      if (found==2) {
	TCHAR *mmisc = StringMallocParse(misc);
	fevent(mmisc);
	free(mmisc);
      } else {
	fevent(TEXT("\0"));
      }
    }

  }
  return (lfound>0);
}


bool ReplayLogger::ReadPoint(double *Time,
			     double *Latitude,
			     double *Longitude,
			     double *Altitude)
{
  TCHAR buffer[200];
  bool found=false;

  while (ReadLine(buffer) && !found) {
    if (ScanBuffer(buffer,Time,Latitude,Longitude,Altitude)) {
      found = true;
    }
  }
  return found;
}



TCHAR ReplayLogger::FileName[MAX_PATH];
bool ReplayLogger::Enabled = false;
double ReplayLogger::TimeScale = 1.0;

bool ReplayLogger::IsEnabled(void) {
  return Enabled;
}


typedef struct _LOGGER_INTERP_POINT
{
  double lat;
  double lon;
  double alt;
  double t;
} LOGGER_INTERP_POINT;

/*
  ps = (1 u u^2 u^3)[0  1 0 0] p0
                    [-t 0 t 0] p1
                    [2t t-3 3-2t -t] p2
                    [-t 2-t t-2 t] p3

*/

class CatmullRomInterpolator {
public:
  CatmullRomInterpolator() {
    Reset();
  }
  void Reset() {
    num=0;
    for (int i=0; i<4; i++) {
      p[i].t= 0;
    }
  }

  LOGGER_INTERP_POINT p[4];

  void Update(double t, double lon, double lat, double alt) {
    if (num<4) { num++; }
    for (int i=0; i<3; i++) {
      p[i].lat = p[i+1].lat;
      p[i].lon = p[i+1].lon;
      p[i].alt = p[i+1].alt;
      p[i].t   = p[i+1].t;
    }
    p[3].lat = lat;
    p[3].lon = lon;
    p[3].alt = alt;
    p[3].t   = t;
  }
  bool Ready() {
    return (num==4);
  }
  double GetSpeed(double time) {
    if (Ready()) {
      double u= (time-p[1].t)/(p[2].t-p[1].t);
      double s0;
      DistanceBearing(p[0].lat, p[0].lon,
                      p[1].lat, p[1].lon, &s0, NULL);
      s0/= (p[1].t-p[0].t);
      double s1;
      DistanceBearing(p[1].lat, p[1].lon,
                      p[2].lat, p[2].lon, &s1, NULL);
      s1/= (p[2].t-p[1].t);
      u = max(0.0,min(1.0,u));
      return s1*u+s0*(1.0-u);
    } else {
      return 0.0;
    }
  }
  void Interpolate(double time, double *lon, double *lat, double *alt) {
    if (!Ready()) {
      *lon = p[num].lon;
      *lat = p[num].lat;
      *alt = p[num].alt;
      return;
    }
    double t=0.98;
    double u= (time-p[1].t)/(p[2].t-p[1].t);

    if (u<0.0) {
      *lat = p[1].lat;
      *lon = p[1].lon;
      *alt = p[1].alt;
      return;
    }
    if (u>1.0) {
      *lat = p[2].lat;
      *lon = p[2].lon;
      *alt = p[2].alt;
      return;
    }

    double u2 = u*u;
    double u3 = u2*u;
    double c[4]= {-t*u3+2*t*u2-t*u,
                  (2-t)*u3+(t-3)*u2+1,
                  (t-2)*u3+(3-2*t)*u2+t*u,
                  t*u3-t*u2};
    /*
    double c[4] = {-t*u+2*t*u2-t*u3,
                   1+(t-3)*u2+(2-t)*u3,
                   t*u+(3-2*t)*u2+(t-2)*u3,
                   -t*u2+t*u3};
    */

    *lat = (p[0].lat*c[0] + p[1].lat*c[1] + p[2].lat*c[2] + p[3].lat*c[3]);
    *lon = (p[0].lon*c[0] + p[1].lon*c[1] + p[2].lon*c[2] + p[3].lon*c[3]);
    *alt = (p[0].alt*c[0] + p[1].alt*c[1] + p[2].alt*c[2] + p[3].alt*c[3]);

  }
  double GetMinTime(void) {
    return p[0].t;
  }
  double GetMaxTime(void) {
    return max(0,max(p[0].t, max(p[1].t, max(p[2].t, p[3].t))));
  }
  double GetAverageTime(void) {
    double tav= 0;
    if (num>0) {
      for (int i=0; i<num; i++) {
        tav += p[i].t/num;
      }
    }
    return tav;
  }
  bool NeedData(double tthis) {
    return (!Ready())||(p[2].t<=tthis+0.1);
  }
private:
  int num;
  double tzero;
};




bool ReplayLogger::UpdateInternal(void) {
  static bool init=true;

  if (!Enabled) {
    init = true;
    ReadLine(NULL); // close file
    Enabled = true;
  }

  static CatmullRomInterpolator cli;

  SYSTEMTIME st;
  GetLocalTime(&st);
  static double time_lstart = 0;

  if (init) {
    time_lstart = 0;
  }
  static double time=0;
  double deltatimereal;
  static double tthis=0;
  static double tlast;

  bool finished = false;

  double timelast = time;
  time = (st.wHour*3600+st.wMinute*60+st.wSecond-time_lstart);
  deltatimereal = time-timelast;

  if (init) {
    time_lstart = time;
    time = 0;
    deltatimereal = 0;
    tthis = 0;
    tlast = tthis;
    cli.Reset();
  }

  tthis += TimeScale*deltatimereal;

  double mintime = cli.GetMinTime(); // li_lat.GetMinTime();
  if (tthis<mintime) { tthis = mintime; }

  // if need a new point
  while (cli.NeedData(tthis)&&(!finished)) {

    double t1, Lat1, Lon1, Alt1;
    finished = !ReadPoint(&t1,&Lat1,&Lon1,&Alt1);

    if (!finished && (t1>0)) {
      cli.Update(t1,Lon1,Lat1,Alt1);
    }
  }

  if (!finished) {

    double LatX, LonX, AltX, SpeedX, BearingX;
    double LatX1, LonX1, AltX1;

    cli.Interpolate(tthis, &LonX, &LatX, &AltX);
    cli.Interpolate(tthis+0.1, &LonX1, &LatX1, &AltX1);

    SpeedX = cli.GetSpeed(tthis);
    DistanceBearing(LatX, LonX, LatX1, LonX1, NULL, &BearingX);

    if ((SpeedX>0) && (LatX != LatX1) && (LonX != LonX1)) {

      LockFlightData();
      if (init) {
        flightstats.Reset();
      }
      GPS_INFO.Latitude = LatX;
      GPS_INFO.Longitude = LonX;
      GPS_INFO.Speed = SpeedX;
      GPS_INFO.TrackBearing = BearingX;
      GPS_INFO.Altitude = AltX;
      GPS_INFO.BaroAltitude = AltX;
      GPS_INFO.Time = tthis;
      UnlockFlightData();
    } else {
      // This is required in case the integrator fails,
      // which can occur due to parsing faults
      tthis = cli.GetMaxTime();
    }
  }

  // quit if finished.
  if (finished) {
    Stop();
  }
  init = false;

  return !finished;
}


void ReplayLogger::Stop(void) {
  ReadLine(NULL); // close the file
  if (Enabled) {
    LockFlightData();
    GPS_INFO.Speed = 0;
    //    GPS_INFO.Time = 0;
    NumLoggerBuffered = 0;
    UnlockFlightData();
  }
  Enabled = false;
}


void ReplayLogger::Start(void) {
  if (Enabled) {
    Stop();
  }
  NumLoggerBuffered = 0;
  flightstats.Reset();
  if (!UpdateInternal()) {
    // TODO couldn't start, give error dialog
    MessageBoxX(hWndMapWindow,
		gettext(TEXT("Could not open IGC file!")),
		gettext(TEXT("Flight replay")),
		MB_OK| MB_ICONINFORMATION);
  }
}


TCHAR* ReplayLogger::GetFilename(void) {
  return FileName;
}


void ReplayLogger::SetFilename(TCHAR *name) {
  if (!name) {
    return;
  }
  if (_tcscmp(FileName,name)!=0) {
    _tcscpy(FileName,name);
  }
}

bool ReplayLogger::Update(void) {
  if (!Enabled)
    return false;

  Enabled = UpdateInternal();
  return Enabled;
}



///////////////////////


FILETIME LogFileDate(TCHAR* filename) {
  FILETIME ft;
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;

  TCHAR asset[MAX_PATH];
  SYSTEMTIME st;
  unsigned short year, month, day, num;
  int matches;
  matches = swscanf(filename,
                    TEXT("%hu-%hu-%hu-%7s-%hu.IGC"),
                    &year,
                    &month,
                    &day,
                    asset,
                    &num);
  if (matches==5) {
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = num;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st,&ft);
    return ft;
  }

  TCHAR cyear, cmonth, cday, cflight;
  matches = _stscanf(filename,
                    TEXT("%c%c%c%4s%c.IGC"),
                    &cyear,
                    &cmonth,
                    &cday,
                    asset,
                    &cflight);
  if (matches==5) {
    int iyear = (int)GPS_INFO.Year;
    int syear = iyear % 10;
    int yearzero = iyear - syear;
    int yearthis = IGCCharToNum(cyear) + yearzero;
    if (yearthis > iyear) {
      yearthis -= 10;
    }
    st.wYear = yearthis;
    st.wMonth = IGCCharToNum(cmonth);
    st.wDay = IGCCharToNum(cday);
    st.wHour = IGCCharToNum(cflight);
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st,&ft);
    return ft;
    // JMW TODO: scan for short filename
    /*
      YMDCXXXF.IGC
      Y: Year, 0 to 9 cycling every 10 years
      M: Month, 1 to 9 then A for 10, B=11, C=12
      D: Day, 1 to 9 then A for 10, B=....
      C: Manuf. code = X
      XXX: Logger ID Alphanum
      F: Flight of day, 1 to 9 then A through Z
    */
  }
  return ft;
}


bool LogFileIsOlder(TCHAR *oldestname, TCHAR *thisname) {
  FILETIME ftold = LogFileDate(oldestname);
  FILETIME ftnew = LogFileDate(thisname);
  return (CompareFileTime(&ftold, &ftnew)>0);
}


bool DeleteOldIGCFile(TCHAR *pathname) {
  HANDLE hFind;  // file handle
  WIN32_FIND_DATA FindFileData;
  TCHAR oldestname[MAX_PATH];
  TCHAR searchpath[MAX_PATH];
  TCHAR fullname[MAX_PATH];
  _stprintf(searchpath, TEXT("%s*"),pathname);

  hFind = FindFirstFile(searchpath, &FindFileData); // find the first file
  if(hFind == INVALID_HANDLE_VALUE) {
    return false;
  }
  if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    if (MatchesExtension(FindFileData.cFileName, TEXT(".igc")) ||
	MatchesExtension(FindFileData.cFileName, TEXT(".IGC"))) {
      // do something...
      _tcscpy(oldestname, FindFileData.cFileName);
    } else {
      return false;
    }
  }
  bool bSearch = true;
  while(bSearch) { // until we finds an entry
    if(FindNextFile(hFind,&FindFileData)) {
      if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
	 (MatchesExtension(FindFileData.cFileName, TEXT(".igc")) ||
	  (MatchesExtension(FindFileData.cFileName, TEXT(".IGC"))))) {
	if (LogFileIsOlder(oldestname,FindFileData.cFileName)) {
	  _tcscpy(oldestname, FindFileData.cFileName);
	  // we have a new oldest name
	}
      }
    } else {
      bSearch = false;
    }
  }
  FindClose(hFind);  // closing file handle

  // now, delete the file...
  _stprintf(fullname, TEXT("%s%s"),pathname,oldestname);
  DeleteFile(fullname);
  return true; // did delete one
}


#define LOGGER_MINFREESTORAGE (250+MINFREESTORAGE)
// JMW note: we want to clear up enough space to save the persistent
// data (85 kb approx) and a new log file

#ifdef DEBUG_IGCFILENAME
TCHAR testtext1[] = TEXT("2007-11-05-XXX-AAA-01.IGC");
TCHAR testtext2[] = TEXT("2007-11-05-XXX-AAA-02.IGC");
TCHAR testtext3[] = TEXT("3BOA1VX2.IGC");
TCHAR testtext4[] = TEXT("5BDX7B31.IGC");
TCHAR testtext5[] = TEXT("3BOA1VX2.IGC");
TCHAR testtext6[] = TEXT("9BDX7B31.IGC");
TCHAR testtext7[] = TEXT("2008-01-05-XXX-AAA-01.IGC");
#endif

bool LoggerClearFreeSpace(void) {
  bool found = true;
  unsigned long kbfree=0;
  TCHAR pathname[MAX_PATH];
  TCHAR subpathname[MAX_PATH];
  int numtries = 0;

  LocalPath(pathname);
#ifdef GNAV
  LocalPath(subpathname,TEXT("logs"));
#else
  LocalPath(subpathname);
#endif

#ifdef DEBUG_IGCFILENAME
  bool retval;
  retval = LogFileIsOlder(testtext1,
                          testtext2);
  retval = LogFileIsOlder(testtext1,
                          testtext3);
  retval = LogFileIsOlder(testtext4,
                          testtext5);
  retval = LogFileIsOlder(testtext6,
                          testtext7);
#endif

  while (found && ((kbfree = FindFreeSpace(pathname))<LOGGER_MINFREESTORAGE)
	 && (numtries++ <100)) {
    /* JMW asking for deleting old files is disabled now --- system
       automatically deletes old files as required
    */

    // search for IGC files, and delete the oldest one
    found = DeleteOldIGCFile(pathname);
    if (!found) {
      found = DeleteOldIGCFile(subpathname);
    }
  }
  return (kbfree>=LOGGER_MINFREESTORAGE);
}

