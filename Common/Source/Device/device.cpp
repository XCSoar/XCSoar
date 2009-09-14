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

// 20070413:sgi add NmeaOut support, allow nmea chaining an double port platforms

#include "Device/device.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "Interface.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Math/FastMath.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "Registry.hpp"
#include "Math/Pressure.h"
#include "Device/devCAI302.h"
#include "Device/devCaiGpsNav.h"
#include "Device/devEW.h"
#include "Device/devAltairPro.h"
#include "Device/devGeneric.h"
#include "Device/devVega.h"
#include "Device/devNmeaOut.h"
#include "Device/devPosiGraph.h"
#include "Device/devBorgeltB50.h"
#include "Device/devVolkslogger.h"
#include "Device/devEWMicroRecorder.h"
#include "Device/devLX.h"
#include "Device/devZander.h"
#include "Device/devFlymasterF1.h"
#include "Device/devXCOM760.h"
#include "Device/devCondor.h"
#include "options.h" /* for LOGGDEVCOMMANDLINE */

// A note about locking.
//  The ComPort RX threads lock using FlightData critical section.
//  ComPort::StopRxThread and ComPort::Close both wait for these threads to
//  exit before returning.  Both these functions are called with the Comm
//  critical section locked.  Therefore, there is a locking dependency upon
//  Comm -> FlightData.
//
//  If someone locks FlightData and then Comm, there is a high possibility
//  of deadlock.  So, FlightData must never be locked after Comm.  Ever.
//  Thankfully WinCE "critical sections" are recursive locks.

#define debugIGNORERESPONCE 0

static  const TCHAR *COMMPort[] = {TEXT("COM1:"),TEXT("COM2:"),TEXT("COM3:"),TEXT("COM4:"),TEXT("COM5:"),TEXT("COM6:"),TEXT("COM7:"),TEXT("COM8:"),TEXT("COM9:"),TEXT("COM10:"),TEXT("COM0:")};
static  const DWORD   dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};


DeviceRegister_t   DeviceRegister[NUMREGDEV];
DeviceDescriptor_t DeviceList[NUMDEV];

DeviceDescriptor_t *pDevPrimaryBaroSource=NULL;
DeviceDescriptor_t *pDevSecondaryBaroSource=NULL;

int DeviceRegisterCount = 0;

static BOOL FlarmDeclare(PDeviceDescriptor_t d, Declaration_t *decl);


// This function is used to determine whether a generic
// baro source needs to be used if available
BOOL devHasBaroSource(void) {
  if (pDevPrimaryBaroSource || pDevSecondaryBaroSource) {
    return TRUE;
  } else {
    return FALSE;
  }
}



BOOL devGetBaroAltitude(double *Value){
  // hack, just return GPS_INFO->BaroAltitude
  if (Value == NULL)
    return(FALSE);
  if (device_blackboard.Basic().BaroAltitudeAvailable)
    *Value = device_blackboard.Basic().BaroAltitude;
  return(TRUE);

  // ToDo
  // more than one baro source may be available
  // eg Altair (w. Logger) and intelligent vario
  // - which source should be used?
  // - whats happen if primary source fails
  // - plausibility check? against second baro? or GPS alt?
  // - whats happen if the diference is too big?

}

BOOL ExpectString(PDeviceDescriptor_t d, const TCHAR *token){

  int i=0, ch;

  if (!d->Com)
    return FALSE;

  while ((ch = d->Com->GetChar()) != EOF){

    if (token[i] == ch)
      i++;
    else
      i=0;

    if ((unsigned)i == _tcslen(token))
      return(TRUE);

  }

  #if debugIGNORERESPONCE > 0
  return(TRUE);
  #endif
  return(FALSE);

}


BOOL devRegister(const DeviceRegister_t *devReg) {
  if (DeviceRegisterCount >= NUMREGDEV)
    return(FALSE);
  DeviceRegister[DeviceRegisterCount] = *devReg;
  DeviceRegisterCount++;
  return(TRUE);
}

BOOL devRegisterGetName(int Index, TCHAR *Name){
  Name[0] = '\0';
  if (Index < 0 || Index >= DeviceRegisterCount)
    return (FALSE);
  _tcscpy(Name, DeviceRegister[Index].Name);
  return(TRUE);
}


BOOL devIsFalseReturn(PDeviceDescriptor_t d){
  (void)d;
  return FALSE;
}

BOOL devIsTrueReturn(PDeviceDescriptor_t d){
  (void)d;
  return TRUE;
}


DeviceRegister_t *devGetDriver(const TCHAR *DevName)
{
  int i;

  for (i = DeviceRegisterCount - 1; i >= 0; i--)
    if (_tcscmp(DeviceRegister[i].Name, DevName) == 0 || i == 0)
      return &DeviceRegister[i];

  return NULL;
}

BOOL devInitOne(PDeviceDescriptor_t dev, int index, const TCHAR *port,
		DWORD speed, PDeviceDescriptor_t &nmeaout)
{
  TCHAR DeviceName[DEVNAMESIZE];

#ifdef _SIM_
    return FALSE;
#endif

  ReadDeviceSettings(index, DeviceName);

  DeviceRegister_t *Driver = devGetDriver(DeviceName);

  if (Driver) {
    ComPort *Com = new ComPort(dev);

    if (!Com->Initialize(port, speed))
      return FALSE;

    memset(dev->Name, 0, sizeof(dev->Name));
    _tcsncpy(dev->Name, Driver->Name, DEVNAMESIZE);

    dev->Driver = Driver;

    dev->Com = Com;

    devOpen(dev, index);

    if (devIsBaroSource(dev)) {
      if (pDevPrimaryBaroSource == NULL) {
        pDevPrimaryBaroSource = dev;
      } else if (pDevSecondaryBaroSource == NULL) {
        pDevSecondaryBaroSource = dev;
      }
    }

    if (nmeaout == NULL && Driver->Flags & (1l << dfNmeaOut)) {
      nmeaout = dev;
    }
  }
  return TRUE;
}

BOOL devInit(LPCTSTR CommandLine){
  int i;
  PDeviceDescriptor_t pDevNmeaOut = NULL;

  for (i=0; i<NUMDEV; i++){
    DeviceList[i].Port = -1;
    DeviceList[i].fhLogFile = NULL;
    DeviceList[i].Name[0] = '\0';
    DeviceList[i].Driver = NULL;
    DeviceList[i].pDevPipeTo = NULL;
  }

  pDevPrimaryBaroSource = NULL;
  pDevSecondaryBaroSource=NULL;

  DWORD PortIndex1, PortIndex2, SpeedIndex1, SpeedIndex2;
#ifdef GNAV
  PortIndex1 = 2; SpeedIndex1 = 5;
  PortIndex2 = 0; SpeedIndex2 = 5;
#else
  PortIndex1 = 0; SpeedIndex1 = 2;
  PortIndex2 = 0; SpeedIndex2 = 2;
#endif
  ReadPort1Settings(&PortIndex1,&SpeedIndex1);
  ReadPort2Settings(&PortIndex2,&SpeedIndex2);

  devInitOne(devA(), 0, COMMPort[PortIndex1], dwSpeed[SpeedIndex1], pDevNmeaOut);

  if (PortIndex1 != PortIndex2)
    devInitOne(devB(), 1, COMMPort[PortIndex2], dwSpeed[SpeedIndex2], pDevNmeaOut);

  CommandLine = LOGGDEVCOMMANDLINE;

  if (CommandLine != NULL){
    TCHAR *pC, *pCe;
    TCHAR wcLogFileName[MAX_PATH];
    TCHAR sTmp[128];

    pC = _tcsstr(CommandLine, TEXT("-logA="));
    if (pC != NULL){
      pC += strlen("-logA=");
      if (*pC == '"'){
        pC++;
        pCe = pC;
        while (*pCe != '"' && *pCe != '\0') pCe++;
      } else{
        pCe = pC;
        while (*pCe != ' ' && *pCe != '\0') pCe++;
      }
      if (pCe != NULL && pCe-1 > pC){

        _tcsncpy(wcLogFileName, pC, pCe-pC);
        wcLogFileName[pCe-pC] = '\0';

        if (devOpenLog(devA(), wcLogFileName)){
          _stprintf(sTmp, TEXT("Device A logs to\r\n%s"), wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(TEXT("Information")),
                      MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp,
                    TEXT("Unable to open log\r\non device A\r\n%s"), wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(TEXT("Error")),
                      MB_OK|MB_ICONWARNING);
        }

      }

    }

    pC = _tcsstr(CommandLine, TEXT("-logB="));
    if (pC != NULL){
      pC += strlen("-logA=");
      if (*pC == '"'){
        pC++;
        pCe = pC;
        while (*pCe != '"' && *pCe != '\0') pCe++;
      } else{
        pCe = pC;
        while (*pCe != ' ' && *pCe != '\0') pCe++;
      }
      if (pCe != NULL && pCe > pC){

        _tcsncpy(wcLogFileName, pC, pCe-pC);
        wcLogFileName[pCe-pC] = '\0';

        if (devOpenLog(devB(), wcLogFileName)){
          _stprintf(sTmp, TEXT("Device B logs to\r\n%s"), wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(TEXT("Information")),
                      MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp, TEXT("Unable to open log\r\non device B\r\n%s"),
                    wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(TEXT("Error")),
                      MB_OK|MB_ICONWARNING);
        }

      }

    }

  }

  if (pDevNmeaOut != NULL){
    if (pDevNmeaOut == devA()){
      devB()->pDevPipeTo = devA();
    }
    if (pDevNmeaOut == devB()){
      devA()->pDevPipeTo = devB();
    }
  }

  return(TRUE);
}


BOOL
devParseNMEA(PDeviceDescriptor_t d, const TCHAR *String, NMEA_INFO *GPS_INFO)
{

  if ((d->fhLogFile != NULL) &&
      (String != NULL) && (_tcslen(String) > 0)) {
    char  sTmp[500];  // temp multibyte buffer
    const TCHAR *pWC = String;
    char  *pC  = sTmp;
    //    static DWORD lastFlush = 0;

    sprintf(pC, "%9u <", (unsigned)GetTickCount());
    pC = sTmp + strlen(sTmp);

    while (*pWC){
      if (*pWC != '\r'){
        *pC = (char)*pWC;
        pC++;
      }
      pWC++;
    }
    *pC++ = '>';
    *pC++ = '\r';
    *pC++ = '\n';
    *pC++ = '\0';

    fputs(sTmp, d->fhLogFile);

  }


  if (d->pDevPipeTo && d->pDevPipeTo->Com) {
    // stream pipe, pass nmea to other device (NmeaOut)
    // TODO code: check TX buffer usage and skip it if buffer is full (outbaudrate < inbaudrate)
    d->pDevPipeTo->Com->WriteString(String);
  }

  if (d->Driver && d->Driver->ParseNMEA)
    if ((d->Driver->ParseNMEA)(d, String, GPS_INFO)) {
      GPS_INFO->Connected = 2;
      return(TRUE);
    }

  if(String[0]=='$') {  // Additional "if" to find GPS strings
    if(NMEAParser::ParseNMEAString(d->Port, String, GPS_INFO)) {
      GPS_INFO->Connected = 2;
      return(TRUE);
    }
  }

  return(FALSE);

}


BOOL devPutMacCready(PDeviceDescriptor_t d, double MacCready)
{
  BOOL result = TRUE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d && d->Driver && d->Driver->PutMacCready)
    result = d->Driver->PutMacCready(d, MacCready);
  mutexComm.Unlock();

  return result;
}

BOOL devPutBugs(PDeviceDescriptor_t d, double Bugs)
{
  BOOL result = TRUE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d && d->Driver && d->Driver->PutBugs)
    result = d->Driver->PutBugs(d, Bugs);
  mutexComm.Unlock();

  return result;
}

BOOL devPutBallast(PDeviceDescriptor_t d, double Ballast)
{
  BOOL result = TRUE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d && d->Driver && d->Driver->PutBallast)
    result = d->Driver->PutBallast(d, Ballast);
  mutexComm.Unlock();

  return result;
}

// Only called from devInit() above which
// is in turn called with mutexComm.Lock
BOOL devOpen(PDeviceDescriptor_t d, int Port){
  BOOL res = TRUE;

  if (d && d->Driver && d->Driver->Open)
    res = d->Driver->Open(d, Port);

  if (res == TRUE)
    d->Port = Port;

  return res;
}

// Tear down methods should always succeed.
// Called from devInit() above under LockComm
// Also called when shutting down via devShutdown()
BOOL devClose(PDeviceDescriptor_t d)
{
  if (d != NULL) {
    if (d->Driver && d->Driver->Close)
      d->Driver->Close(d);

    ComPort *Com = d->Com;
    d->Com = NULL;

    if (Com) {
      Com->Close();
      delete Com;
    }
  }

  return TRUE;
}

BOOL devLinkTimeout(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->Driver && d->Driver->LinkTimeout != NULL)
        (d->Driver->LinkTimeout)(d);
    }
    result = TRUE;
  } else {
    if (d->Driver && d->Driver->LinkTimeout != NULL)
      result = d->Driver->LinkTimeout(d);
  }
  mutexComm.Unlock();

  return FALSE;
}


BOOL devPutVoice(PDeviceDescriptor_t d, TCHAR *Sentence)
{
  BOOL result = FALSE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->Driver && d->Driver->PutVoice)
        d->Driver->PutVoice(d, Sentence);
    }
    result = TRUE;
  } else {
    if (d->Driver && d->Driver->PutVoice)
      result = d->Driver->PutVoice(d, Sentence);
  }
  mutexComm.Unlock();

  return FALSE;
}

BOOL devDeclare(PDeviceDescriptor_t d, Declaration_t *decl)
{
  BOOL result = FALSE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d) {
    if ((d->Driver) && (d->Driver->Declare != NULL))
      result = d->Driver->Declare(d, decl);

    if (NMEAParser::PortIsFlarm(d->Port))
      result |= FlarmDeclare(d, decl);
  }
  mutexComm.Unlock();

  return result;
}

BOOL devIsLogger(PDeviceDescriptor_t d)
{
  bool result = false;

  mutexComm.Lock();
  if (d && d->Driver) {
    if (d->Driver->IsLogger)
      result = d->Driver->IsLogger(d);
    else
      result = d->Driver->Flags & drfLogger ? TRUE : FALSE;
    if (!result)
      result |= NMEAParser::PortIsFlarm(d->Port);
  }
  mutexComm.Unlock();

  return result;
}

BOOL devIsGPSSource(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  mutexComm.Lock();
  if (d && d->Driver) {
    if (d->Driver->IsGPSSource)
      result = d->Driver->IsGPSSource(d);
    else
      result = d->Driver->Flags & drfGPS ? TRUE : FALSE;
  }
  mutexComm.Unlock();

  return result;
}

BOOL devIsBaroSource(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  mutexComm.Lock();
  if (d && d->Driver) {
    if (d->Driver->IsBaroSource)
      result = d->Driver->IsBaroSource(d);
    else
      result = d->Driver->Flags & drfBaroAlt ? TRUE : FALSE;
  }
  mutexComm.Unlock();

  return result;
}

BOOL devIsRadio(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  mutexComm.Lock();
  if (d && d->Driver) {
    result = d->Driver->Flags & drfRadio ? TRUE : FALSE;
  }
  mutexComm.Unlock();

  return result;
}


BOOL devIsCondor(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  mutexComm.Lock();
  if (d && d->Driver) {
    result = d->Driver->Flags & drfCondor ? TRUE : FALSE;
  }
  mutexComm.Unlock();

  return result;
}



BOOL devOpenLog(PDeviceDescriptor_t d, const TCHAR *FileName){
  if (d != NULL){
    d->fhLogFile = _tfopen(FileName, TEXT("a+b"));
    return(d->fhLogFile != NULL);
  } else
    return(FALSE);
}

BOOL devCloseLog(PDeviceDescriptor_t d){
  if (d != NULL && d->fhLogFile != NULL){
    fclose(d->fhLogFile);
    return(TRUE);
  } else
    return(FALSE);
}

BOOL devPutQNH(DeviceDescriptor_t *d, double NewQNH)
{
  BOOL result = FALSE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->Driver && d->Driver->PutQNH)
        d->Driver->PutQNH(d, NewQNH);
    }
    result = TRUE;
  } else {
    if (d->Driver && d->Driver->PutQNH)
      result = d->Driver->PutQNH(d, NewQNH);
  }
  mutexComm.Unlock();

  return FALSE;
}

void devTick()
{
  int i;

  mutexComm.Lock();
  for (i = 0; i < NUMDEV; i++) {
    DeviceDescriptor_t *d = &DeviceList[i];
    if (!d->Driver)
      continue;

    d->ticker = !d->ticker;

    // write settings to vario every second
    if (d->ticker && d->Driver->OnSysTicker)
      d->Driver->OnSysTicker(d);
  }
  mutexComm.Unlock();
}

static void devFormatNMEAString(TCHAR *dst, size_t sz, const TCHAR *text)
{
  BYTE chk;
  int i, len = _tcslen(text);

  for (chk = i = 0; i < len; i++)
    chk ^= (BYTE)text[i];

  _sntprintf(dst, sz, TEXT("$%s*%02X\r\n"), text, chk);
}

void devWriteNMEAString(PDeviceDescriptor_t d, const TCHAR *text)
{
  TCHAR tmp[512];

  devFormatNMEAString(tmp, 512, text);

  mutexComm.Lock();
  if (d->Com)
    d->Com->WriteString(tmp);
  mutexComm.Unlock();
}

void VarioWriteNMEA(const TCHAR *text)
{
  TCHAR tmp[512];

  devFormatNMEAString(tmp, 512, text);

  mutexComm.Lock();
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, TEXT("Vega")) == 0)
      if (DeviceList[i].Com)
        DeviceList[i].Com->WriteString(tmp);
  mutexComm.Unlock();
}

PDeviceDescriptor_t devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, TEXT("Vega")) == 0)
      return &DeviceList[i];
  return NULL;
}


BOOL devPutVolume(PDeviceDescriptor_t d, int Volume)
{
  BOOL result = TRUE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d && d->Driver && d->Driver->PutVolume != NULL)
    result = d->Driver->PutVolume(d, Volume);
  mutexComm.Unlock();

  return result;
}

BOOL devPutFreqActive(PDeviceDescriptor_t d, double Freq)
{
  BOOL result = TRUE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d && d->Driver && d->Driver->PutFreqActive != NULL)
    result = d->Driver->PutFreqActive(d, Freq);
  mutexComm.Unlock();

  return result;
}

BOOL devPutFreqStandby(PDeviceDescriptor_t d, double Freq)
{
  BOOL result = TRUE;

#ifdef _SIM_
  return TRUE;
#endif

  mutexComm.Lock();
  if (d && d->Driver && d->Driver->PutFreqStandby != NULL)
    result = d->Driver->PutFreqStandby(d, Freq);
  mutexComm.Unlock();

  return result;
}


static BOOL
FlarmDeclareSetGet(PDeviceDescriptor_t d, TCHAR *Buffer) {
  //devWriteNMEAString(d, Buffer);

  TCHAR tmp[512];

  _sntprintf(tmp, 512, TEXT("$%s\r\n"), Buffer);

  if (d->Com)
    d->Com->WriteString(tmp);

  Buffer[6]= _T('A');
  if (!ExpectString(d, Buffer)){
    return FALSE;
  } else {
    return TRUE;
  }
}


BOOL FlarmDeclare(PDeviceDescriptor_t d, Declaration_t *decl){
  BOOL result = TRUE;

  TCHAR Buffer[256];

  d->Com->StopRxThread();
  d->Com->SetRxTimeout(500);                     // set RX timeout to 500[ms]

  _stprintf(Buffer,TEXT("PFLAC,S,PILOT,%s"),decl->PilotName);
  if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;

  _stprintf(Buffer,TEXT("PFLAC,S,GLIDERID,%s"),decl->AircraftRego);
  if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;

  _stprintf(Buffer,TEXT("PFLAC,S,GLIDERTYPE,%s"),decl->AircraftType);
  if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;

  _stprintf(Buffer,TEXT("PFLAC,S,NEWTASK,Task"));
  if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;

  _stprintf(Buffer,TEXT("PFLAC,S,ADDWP,0000000N,00000000E,TAKEOFF"));
  if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;

  for (int i = 0; i < decl->num_waypoints; i++) {
    int DegLat, DegLon;
    double tmp, MinLat, MinLon;
    char NoS, EoW;

    tmp = decl->waypoint[i]->Location.Latitude;
    NoS = 'N';
    if(tmp < 0)
      {
	NoS = 'S';
	tmp = -tmp;
      }
    DegLat = (int)tmp;
    MinLat = (tmp - DegLat) * 60 * 1000;

    tmp = decl->waypoint[i]->Location.Longitude;
    EoW = 'E';
    if(tmp < 0)
      {
	EoW = 'W';
	tmp = -tmp;
      }
    DegLon = (int)tmp;
    MinLon = (tmp - DegLon) * 60 * 1000;

    _stprintf(Buffer,
	      TEXT("PFLAC,S,ADDWP,%02d%05.0f%c,%03d%05.0f%c,%s"),
	      DegLat, MinLat, NoS, DegLon, MinLon, EoW,
	      decl->waypoint[i]->Name);
    if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;
  }

  _stprintf(Buffer,TEXT("PFLAC,S,ADDWP,0000000N,00000000E,LANDING"));
  if (!FlarmDeclareSetGet(d,Buffer)) result = FALSE;

  // PFLAC,S,KEY,VALUE
  // Expect
  // PFLAC,A,blah
  // PFLAC,,COPIL:
  // PFLAC,,COMPID:
  // PFLAC,,COMPCLASS:

  // PFLAC,,NEWTASK:
  // PFLAC,,ADDWP:

  // TODO bug: JMW, FLARM Declaration checks
  // Note: FLARM must be power cycled to activate a declaration!
  // Only works on IGC approved devices
  // Total data size must not surpass 183 bytes
  // probably will issue PFLAC,ERROR if a problem?

  d->Com->SetRxTimeout(0);                       // clear timeout
  d->Com->StartRxThread();                       // restart RX thread

  return result;
}

void devStartup(LPTSTR lpCmdLine)
{
  StartupStore(TEXT("Register serial devices\n"));

  // ... register all supported devices
  // IMPORTANT: ADD NEW ONES TO BOTTOM OF THIS LIST
  genRegister(); // MUST BE FIRST
  cai302Register();
  ewRegister();
  atrRegister();
  vgaRegister();
  caiGpsNavRegister();
  nmoRegister();
  pgRegister();
  b50Register();
  vlRegister();
  ewMicroRecorderRegister();
  lxRegister();
  zanderRegister();
  flymasterf1Register();
  xcom760Register();
  condorRegister();

  devInit(lpCmdLine);
}

void devShutdown()
{
  int i;

  // Stop COM devices
  StartupStore(TEXT("Stop COM devices\n"));

  for (i=0; i<NUMDEV; i++){
    devClose(&DeviceList[i]);
    devCloseLog(&DeviceList[i]);
  }
}


////////////////////////////////////////////////////////////////////////
void devRestart() {
#ifndef _SIM_
  /*
#ifdef WINDOWSPC
  static bool first = true;
  if (!first) {
    NMEAParser::Reset();
    return;
  }
  first = false;
#endif
  */
  StartupStore(TEXT("RestartCommPorts\n"));

  mutexComm.Lock();

  devShutdown();
  NMEAParser::Reset();

  devInit(TEXT(""));

  mutexComm.Unlock();
#endif
}
