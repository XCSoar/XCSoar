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
  FlushFileBuffers(hFile);

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
  FlushFileBuffers(hFile);
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

  FlushFileBuffers(hFile);

  CloseHandle(hFile);
}


void EndDeclaration(void)
{
  // JMW TODO: this is causing problems with some analysis software
  // maybe it's because the date and location fields are bogus
  char start[] = "C0000000N00000000ELANDING (not defined)\r\n";
  HANDLE hFile;
  DWORD dwBytesRead;

  hFile = CreateFile(szLoggerFileName, GENERIC_WRITE,
		     FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  SetFilePointer(hFile, 0, NULL, FILE_END);
  WriteFile(hFile, start, strlen(start), &dwBytesRead,(OVERLAPPED *)NULL);
  FlushFileBuffers(hFile);

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

	  if (LoggerClearFreeSpace()) {
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


#include "Parser.h"

extern NMEA_INFO GPS_INFO;

TCHAR ReplayLogger::FileName[MAX_PATH];
bool ReplayLogger::Enabled = false;
double ReplayLogger::TimeScale = 1.0;

bool ReplayLogger::IsEnabled(void) {
  return Enabled;
}


class ReplayLoggerInterpolator {
public:
  void Reset() {
    x0=0; x1=0; x2=0;
    t0=0; t1=0; t2=0;
    a=0; b=0; c=0;
    num = 0;
    tzero = 0;
  }
  void Update(double t, double x) {
    if (num==0) {
      tzero = t;
    }
    t0=t1; x0=x1;
    t1=t2; x1=x2;
    t2=t-tzero; x2=x;
    if (num<3) {
      num++;
    } else {
      UpdateInterpolator();
    }
  }
  bool Ready() {
    return (num==3);
  }
  double Interpolate(double t) {
    if (!Ready()) {
      return x0;
    }
    double tz= t-tzero;
    return a*tz*tz+b*tz+c;
  }
  double GetMinTime(void) {
    return t0+tzero;
  }
  double GetMaxTime(void) {
    return t2+tzero;
  }
  double GetAverageTime(void) {
    return (t0+t1+t2)/3+tzero;
  }
private:
  double t0, t1, t2;
  double x0, x1, x2;
  double a, b, c;
  int num;
  bool first;
  double tzero;

  void UpdateInterpolator() {
    double d = t0*t0*(t1-t2)+t1*t1*(t2-t0)+t2*t2*(t0-t1);
    if (d == 0.0)
      {
	a=0;
      }
    else
      {
	a=((t1-t2)*(x0-x2)+(t2-t0)*(x1-x2))/d;
      }
    d = t1-t2;
    if (d == 0.0)
      {
	b=0;
      }
    else
      {
	b = (x1-x2-a*(t1*t1-t2*t2))/d;
      }
    c = (x2 - a*t2*t2 - b*t2);
  }
};


bool ReplayLogger::UpdateInternal(void) {
  static bool init=true;

  if (!Enabled) {
    init = true;
    ReadLine(NULL); // close file
    Enabled = true;
  }

  static ReplayLoggerInterpolator li_lat;
  static ReplayLoggerInterpolator li_lon;
  static ReplayLoggerInterpolator li_alt;

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

    li_lat.Reset();
    li_lon.Reset();
    li_alt.Reset();
  }

  tthis += TimeScale*deltatimereal;

  double mintime = li_lat.GetMinTime();
  if (tthis<mintime) { tthis = mintime; }
  /*
  if (tlast>tthis) {
    tlast = tthis;
  }
  if ((int)tthis-(int)tlast<1) {
    return true;
  }
  tlast = tthis;
  */

  // if need a new point
  while (
	 (!li_lat.Ready()||(li_lat.GetAverageTime()< tthis))
	 &&(!finished)) {

    double t1, Lat1, Lon1, Alt1;
    finished = !ReadPoint(&t1,&Lat1,&Lon1,&Alt1);

    if (!finished) {
      li_lat.Update(t1,Lat1);
      li_lon.Update(t1,Lon1);
      li_alt.Update(t1,Alt1);
    }
  }

  if (!finished) {

    double LatX, LonX, AltX, SpeedX, BearingX;
    double LatX1, LonX1;

    AltX = li_alt.Interpolate(tthis);

    LatX = li_lat.Interpolate(tthis);
    LonX = li_lon.Interpolate(tthis);

    LatX1 = li_lat.Interpolate(tthis+1);
    LonX1 = li_lon.Interpolate(tthis+1);

    SpeedX = Distance(LatX, LonX, LatX1, LonX1);
    BearingX = Bearing(LatX, LonX, LatX1, LonX1);

    if (SpeedX>0) {
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
      tthis=li_lat.GetMaxTime();
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
  Enabled = false;
}


void ReplayLogger::Start(void) {
  if (Enabled) {
    Stop();
  }
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
  int matches = swscanf(filename,
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


#define MINFREESTORAGE 500
// 500 kb must be free for logger to be active this is based on rough
// estimate that a long flight will detailed logging is about 200k,
// and we want to leave a little free.

bool LoggerClearFreeSpace(void) {
  bool found = true;
  unsigned long kbfree;
  TCHAR pathname[MAX_PATH];
  int numtries = 0;
  _tcscpy(pathname,LocalPath());

  while (found && ((kbfree = FindFreeSpace(pathname))<MINFREESTORAGE)
	 && (numtries<100)) {
    if (numtries==0)
      if(MessageBoxX(hWndMapWindow,
		     gettext(TEXT("Insufficient free storage, delete old IGC files?")),
		     gettext(TEXT("Logger")),
		     MB_YESNO|MB_ICONQUESTION) != IDYES)
	return false;

    // search for IGC files, and delete the oldest one
    found = DeleteOldIGCFile(pathname);
    numtries++;
  }
  return (kbfree>MINFREESTORAGE);
}
