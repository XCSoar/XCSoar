/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Thread/Mutex.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "Interface.hpp"
#include "Dialogs/Message.hpp"
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
#include "Asset.hpp"

#include <assert.h>

static Mutex mutexComm;

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

const struct DeviceRegister *const DeviceRegister[] = {
  // IMPORTANT: ADD NEW ONES TO BOTTOM OF THIS LIST
  &genDevice, // MUST BE FIRST
  &cai302Device,
  &ewDevice,
  &atrDevice,
  &vgaDevice,
  &caiGpsNavDevice,
  &nmoDevice,
  &pgDevice,
  &b50Device,
  &vlDevice,
  &ewMicroRecorderDevice,
  &lxDevice,
  &zanderDevice,
  &flymasterf1Device,
  &xcom760Device,
  &condorDevice,
  NULL
};

enum {
  DeviceRegisterCount = sizeof(DeviceRegister) / sizeof(DeviceRegister[0]) - 1
};

struct DeviceDescriptor DeviceList[NUMDEV];

struct DeviceDescriptor *pDevPrimaryBaroSource;
struct DeviceDescriptor *pDevSecondaryBaroSource;

static bool
FlarmDeclare(struct DeviceDescriptor *d, const struct Declaration *decl);


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

BOOL ExpectString(struct DeviceDescriptor *d, const TCHAR *token){

  int i=0, ch;

  assert(d != NULL);
  assert(token != NULL);

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

BOOL devRegisterGetName(int Index, TCHAR *Name){
  Name[0] = '\0';
  if (Index < 0 || Index >= DeviceRegisterCount)
    return (FALSE);
  _tcscpy(Name, DeviceRegister[Index]->Name);
  return(TRUE);
}

BOOL
devIsFalseReturn(const struct DeviceDescriptor *d)
{
  (void)d;
  return FALSE;
}

BOOL
devIsTrueReturn(const struct DeviceDescriptor *d)
{
  (void)d;
  return TRUE;
}

static const struct DeviceRegister *
devGetDriver(const TCHAR *DevName)
{
  int i;

  for (i = DeviceRegisterCount - 1; i >= 0; i--)
    if (_tcscmp(DeviceRegister[i]->Name, DevName) == 0 || i == 0)
      return DeviceRegister[i];

  return NULL;
}

static bool
devOpenLog(struct DeviceDescriptor *d, const TCHAR *FileName);

static bool
devInitOne(struct DeviceDescriptor *dev, int index, const TCHAR *port,
           DWORD speed, struct DeviceDescriptor *&nmeaout)
{
  TCHAR DeviceName[DEVNAMESIZE];

  assert(dev != NULL);

  if (is_simulator())
    return FALSE;

  ReadDeviceSettings(index, DeviceName);

  const struct DeviceRegister *Driver = devGetDriver(DeviceName);

  if (Driver) {
    ComPort *Com = new ComPort(dev);

    if (!Com->Initialize(port, speed))
      return FALSE;

    memset(dev->Name, 0, sizeof(dev->Name));
    _tcsncpy(dev->Name, Driver->Name, DEVNAMESIZE);

    dev->Driver = Driver;

    dev->Com = Com;

    dev->Open(index);

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

static BOOL
devInit(LPCTSTR CommandLine)
{
  int i;
  struct DeviceDescriptor *pDevNmeaOut = NULL;

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
devParseNMEA(struct DeviceDescriptor *d, const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  assert(d != NULL);
  assert(String != NULL);
  assert(GPS_INFO != NULL);

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

bool
DeviceDescriptor::Open(int _port)
{
  Port = _port;

  return Driver != NULL && Driver->Open != NULL
    ? Driver->Open(this, Port)
    : true;
}

void
DeviceDescriptor::Close()
{
  if (Driver != NULL && Driver->Close != NULL)
    Driver->Close(this);

  ComPort *OldCom = Com;
  Com = NULL;

  if (OldCom != NULL) {
    OldCom->Close();
    delete OldCom;
  }
}

bool
DeviceDescriptor::IsLogger() const
{
  return Driver != NULL &&
    ((Driver->IsLogger != NULL
      ? Driver->IsLogger(this)
      : (Driver->Flags & drfLogger) != 0) ||
     NMEAParser::PortIsFlarm(Port));
}

bool
DeviceDescriptor::IsGPSSource() const
{
  return Driver != NULL &&
    (Driver->IsGPSSource != NULL
     ? Driver->IsGPSSource(this)
     : (Driver->Flags & drfGPS) != 0);
}

bool
DeviceDescriptor::IsBaroSource() const
{
  return Driver != NULL &&
    (Driver->IsBaroSource != NULL
     ? Driver->IsBaroSource(this)
     : (Driver->Flags & drfBaroAlt) != 0);
}

bool
DeviceDescriptor::PutMcCready(double mc_cready)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutMacCready != NULL)
    result = Driver->PutMacCready(this, mc_cready);

  return result;
}

bool
DeviceDescriptor::PutBugs(double bugs)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutBugs != NULL)
    result = Driver->PutBugs(this, bugs);

  return result;
}

bool
DeviceDescriptor::PutBallast(double ballast)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutBallast != NULL)
    result = Driver->PutBallast(this, ballast);

  return result;
}

bool
DeviceDescriptor::PutVolume(int volume)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutVolume != NULL)
    result = Driver->PutVolume(this, volume);

  return result;
}

bool
DeviceDescriptor::PutActiveFrequency(double frequency)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutFreqActive != NULL)
    result = Driver->PutFreqActive(this, frequency);

  return result;
}

bool
DeviceDescriptor::PutStandbyFrequency(double frequency)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutFreqStandby != NULL)
    result = Driver->PutFreqStandby(this, frequency);

  return result;
}

bool
DeviceDescriptor::PutQNH(double qnh)
{
  BOOL result = TRUE;

  if (Driver != NULL && Driver->PutQNH != NULL)
    result = Driver->PutQNH(this, qnh);

  return result;
}

bool
DeviceDescriptor::PutVoice(const TCHAR *sentence)
{
  BOOL result = TRUE;

  assert(sentence != NULL);

  if (Driver != NULL && Driver->PutVoice != NULL)
    result = Driver->PutVoice(this, sentence);

  return result;
}

void
DeviceDescriptor::LinkTimeout()
{
  if (Driver != NULL && Driver->LinkTimeout != NULL)
    Driver->LinkTimeout(this);
}

bool
DeviceDescriptor::Declare(const struct Declaration *declaration)
{
  if (Driver == NULL)
    return false;

  bool result = Driver->Declare != NULL &&
    Driver->Declare(this, declaration);

  if (NMEAParser::PortIsFlarm(Port))
    result = FlarmDeclare(this, declaration) || result;

  return result;
}

void
DeviceDescriptor::OnSysTicker()
{
    if (Driver == NULL)
      return;

    ticker = !ticker;

    // write settings to vario every second
    if (ticker && Driver->OnSysTicker != NULL)
      Driver->OnSysTicker(this);
}

bool
devDeclare(struct DeviceDescriptor *d, const struct Declaration *decl)
{
  BOOL result = FALSE;

  assert(d != NULL);

  if (is_simulator())
    return true;

  mutexComm.Lock();
  if (d != NULL)
    d->Declare(decl);
  mutexComm.Unlock();

  return result;
}

BOOL devIsLogger(const struct DeviceDescriptor *d)
{
  bool result = false;

  assert(d != NULL);

  mutexComm.Lock();
  if (d != NULL)
    result = d->IsLogger();
  mutexComm.Unlock();

  return result;
}

BOOL devIsGPSSource(const struct DeviceDescriptor *d)
{
  BOOL result = FALSE;

  assert(d != NULL);

  mutexComm.Lock();
  if (d != NULL)
    result = d->IsGPSSource();
  mutexComm.Unlock();

  return result;
}

BOOL devIsBaroSource(const struct DeviceDescriptor *d)
{
  BOOL result = FALSE;

  assert(d != NULL);

  mutexComm.Lock();
  if (d != NULL)
    result = d->IsBaroSource();
  mutexComm.Unlock();

  return result;
}

BOOL devIsRadio(const struct DeviceDescriptor *d)
{
  BOOL result = FALSE;

  assert(d != NULL);

  mutexComm.Lock();
  if (d && d->Driver) {
    result = d->Driver->Flags & drfRadio ? TRUE : FALSE;
  }
  mutexComm.Unlock();

  return result;
}


BOOL devIsCondor(const struct DeviceDescriptor *d)
{
  BOOL result = FALSE;

  assert(d != NULL);

  mutexComm.Lock();
  if (d && d->Driver) {
    result = d->Driver->Flags & drfCondor ? TRUE : FALSE;
  }
  mutexComm.Unlock();

  return result;
}

static bool
devOpenLog(struct DeviceDescriptor *d, const TCHAR *FileName)
{
  assert(d != NULL);

  d->fhLogFile = _tfopen(FileName, TEXT("a+b"));
  return d->fhLogFile != NULL;
}

static bool
devCloseLog(struct DeviceDescriptor *d)
{
  assert(d != NULL);

  if (d->fhLogFile != NULL){
    fclose(d->fhLogFile);
    return(TRUE);
  } else
    return(FALSE);
}

void devTick()
{
  int i;

  mutexComm.Lock();
  for (i = 0; i < NUMDEV; i++) {
    struct DeviceDescriptor *d = &DeviceList[i];
    d->OnSysTicker();
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

void devWriteNMEAString(struct DeviceDescriptor *d, const TCHAR *text)
{
  TCHAR tmp[512];

  assert(d != NULL);

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

struct DeviceDescriptor *devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, TEXT("Vega")) == 0)
      return &DeviceList[i];
  return NULL;
}

static BOOL
FlarmDeclareSetGet(struct DeviceDescriptor *d, TCHAR *Buffer) {
  assert(d != NULL);

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


bool
FlarmDeclare(struct DeviceDescriptor *d, const struct Declaration *decl)
{
  assert(d != NULL);

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

void AllDevicesPutMcCready(double mc_cready)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutMcCready(mc_cready);
}

void AllDevicesPutBugs(double bugs)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutBugs(bugs);
}

void AllDevicesPutBallast(double ballast)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutBallast(ballast);
}

void AllDevicesPutVolume(int volume)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutBallast(volume);
}

void AllDevicesPutActiveFrequency(double frequency)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutActiveFrequency(frequency);
}

void AllDevicesPutStandbyFrequency(double frequency)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutStandbyFrequency(frequency);
}

void AllDevicesPutQNH(double qnh)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutQNH(qnh);
}

void AllDevicesPutVoice(const TCHAR *sentence)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutVoice(sentence);
}

void AllDevicesLinkTimeout()
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].LinkTimeout();
}

void devStartup(LPTSTR lpCmdLine)
{
  StartupStore(TEXT("Register serial devices\n"));

  devInit(lpCmdLine);
}

void devShutdown()
{
  int i;

  // Stop COM devices
  StartupStore(TEXT("Stop COM devices\n"));

  for (i=0; i<NUMDEV; i++){
    DeviceList[i].Close();
    devCloseLog(&DeviceList[i]);
  }
}

void devRestart() {
  if (is_simulator())
    return;

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
}

void devConnectionMonitor()
{
  mutexComm.Lock();
  NMEAParser::UpdateMonitor();
  mutexComm.Unlock();
}
