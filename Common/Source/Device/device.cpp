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
#include "Device/Driver.hpp"
#include "Device/FLARM.hpp"
#include "Thread/Mutex.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "Registry.hpp"
#include "Device/Driver/CAI302.hpp"
#include "Device/Driver/CaiGpsNav.hpp"
#include "Device/Driver/EW.hpp"
#include "Device/Driver/AltairPro.hpp"
#include "Device/Driver/Generic.hpp"
#include "Device/Driver/Vega.hpp"
#include "Device/Driver/NmeaOut.hpp"
#include "Device/Driver/PosiGraph.hpp"
#include "Device/Driver/BorgeltB50.hpp"
#include "Device/Driver/Volkslogger.hpp"
#include "Device/Driver/EWMicroRecorder.hpp"
#include "Device/Driver/LX.hpp"
#include "Device/Driver/Zander.hpp"
#include "Device/Driver/FlymasterF1.hpp"
#include "Device/Driver/XCOM760.hpp"
#include "Device/Driver/Condor.hpp"
#include "NMEA/Checksum.h"
#include "options.h" /* for LOGGDEVCOMMANDLINE */
#include "Asset.hpp"
#include "StringUtil.hpp"

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

static const TCHAR *const COMMPort[] = {
  _T("COM1:"),
  _T("COM2:"),
  _T("COM3:"),
  _T("COM4:"),
  _T("COM5:"),
  _T("COM6:"),
  _T("COM7:"),
  _T("COM8:"),
  _T("COM9:"),
  _T("COM10:"),
  _T("COM0:"),
};

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

static struct DeviceDescriptor *pDevPrimaryBaroSource;
static struct DeviceDescriptor *pDevSecondaryBaroSource;

// This function is used to determine whether a generic
// baro source needs to be used if available
bool
devHasBaroSource(void)
{
  return pDevPrimaryBaroSource || pDevSecondaryBaroSource;
}

bool
devGetBaroAltitude(double *Value)
{
  // hack, just return GPS_INFO->BaroAltitude
  if (Value == NULL)
    return false;
  if (device_blackboard.Basic().BaroAltitudeAvailable)
    *Value = device_blackboard.Basic().BaroAltitude;
  return true;

  // ToDo
  // more than one baro source may be available
  // eg Altair (w. Logger) and intelligent vario
  // - which source should be used?
  // - whats happen if primary source fails
  // - plausibility check? against second baro? or GPS alt?
  // - whats happen if the diference is too big?

}

bool
devRegisterGetName(int Index, TCHAR *Name)
{
  Name[0] = '\0';
  if (Index < 0 || Index >= DeviceRegisterCount)
    return false;
  _tcscpy(Name, DeviceRegister[Index]->Name);
  return true;
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
    return false;

  ReadDeviceSettings(index, DeviceName);

  const struct DeviceRegister *Driver = devGetDriver(DeviceName);

  if (Driver) {
    ComPort *Com = new ComPort(dev);

    if (!Com->Initialize(port, speed))
      return false;

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
  return true;
}

static bool
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
  if (is_altair()) {
    PortIndex1 = 2; SpeedIndex1 = 5;
    PortIndex2 = 0; SpeedIndex2 = 5;
  } else {
    PortIndex1 = 0; SpeedIndex1 = 2;
    PortIndex2 = 0; SpeedIndex2 = 2;
  }

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

    pC = _tcsstr(CommandLine, _T("-logA="));
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
          _stprintf(sTmp, _T("Device A logs to\r\n%s"), wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(_T("Information")),
                      MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp,
                    _T("Unable to open log\r\non device A\r\n%s"), wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(_T("Error")),
                      MB_OK|MB_ICONWARNING);
        }

      }

    }

    pC = _tcsstr(CommandLine, _T("-logB="));
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
          _stprintf(sTmp, _T("Device B logs to\r\n%s"), wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(_T("Information")),
                      MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp, _T("Unable to open log\r\non device B\r\n%s"),
                    wcLogFileName);
          MessageBoxX (sTmp,
                      gettext(_T("Error")),
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

  return true;
}

bool
DeviceDescriptor::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO)
{
  assert(String != NULL);
  assert(GPS_INFO != NULL);

  if (fhLogFile != NULL &&
      String != NULL && !string_is_empty(String)) {
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

    fputs(sTmp, fhLogFile);
  }


  if (pDevPipeTo && pDevPipeTo->Com) {
    // stream pipe, pass nmea to other device (NmeaOut)
    // TODO code: check TX buffer usage and skip it if buffer is full (outbaudrate < inbaudrate)
    pDevPipeTo->Com->WriteString(String);
  }

  if (device != NULL && device->ParseNMEA(String, GPS_INFO,
                                          this == pDevPrimaryBaroSource)) {
    GPS_INFO->Connected = 2;
    return true;
  }

  if(String[0]=='$') {  // Additional "if" to find GPS strings
    if(NMEAParser::ParseNMEAString(Port, String, GPS_INFO)) {
      GPS_INFO->Connected = 2;
      return true;
    }
  }

  return false;
}

bool
DeviceDescriptor::Open(int _port)
{
  Port = _port;

  if (Driver == NULL)
    return false;

  assert(Driver->CreateOnComPort != NULL);

  device = Driver->CreateOnComPort(Com);
  if (!device->Open()) {
    delete device;
    device = NULL;
    return false;
  }

  return true;
}

void
DeviceDescriptor::Close()
{
  if (device != NULL) {
    delete device;
    device = NULL;
  }

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
    ((Driver->Flags & drfLogger) != 0 ||
     (device != NULL && device->IsLogger()) ||
     NMEAParser::PortIsFlarm(Port));
}

bool
DeviceDescriptor::IsGPSSource() const
{
  return Driver != NULL &&
    ((Driver->Flags & drfGPS) != 0 ||
     (device != NULL && device->IsGPSSource()));
}

bool
DeviceDescriptor::IsBaroSource() const
{
  return Driver != NULL &&
    ((Driver->Flags & drfBaroAlt) != 0 ||
     (device != NULL && device->IsBaroSource()));
}

bool
DeviceDescriptor::PutMacCready(double MacCready)
{
  return device != NULL
    ? device->PutMacCready(MacCready)
    : true;
}

bool
DeviceDescriptor::PutBugs(double bugs)
{
  return device != NULL
    ? device->PutBugs(bugs)
    : true;
}

bool
DeviceDescriptor::PutBallast(double ballast)
{
  return device != NULL
    ? device->PutBallast(ballast)
    : true;
}

bool
DeviceDescriptor::PutVolume(int volume)
{
  return device != NULL
    ? device->PutVolume(volume)
    : true;
}

bool
DeviceDescriptor::PutActiveFrequency(double frequency)
{
  return device != NULL
    ? device->PutActiveFrequency(frequency)
    : true;
}

bool
DeviceDescriptor::PutStandbyFrequency(double frequency)
{
  return device != NULL
    ? device->PutStandbyFrequency(frequency)
    : true;
}

bool
DeviceDescriptor::PutQNH(double qnh)
{
  return device != NULL
    ? device->PutQNH(qnh)
    : true;
}

bool
DeviceDescriptor::PutVoice(const TCHAR *sentence)
{
  return device != NULL
    ? device->PutVoice(sentence)
    : true;
}

void
DeviceDescriptor::LinkTimeout()
{
  if (device != NULL)
    device->LinkTimeout();
}

bool
DeviceDescriptor::Declare(const struct Declaration *declaration)
{
  bool result = device != NULL &&
    device->Declare(declaration);

  if (NMEAParser::PortIsFlarm(Port))
    result = FlarmDeclare(Com, declaration) || result;

  return result;
}

void
DeviceDescriptor::OnSysTicker()
{
  if (device == NULL)
    return;

  ticker = !ticker;
  if (ticker)
    // write settings to vario every second
    device->OnSysTicker();
}

bool
devDeclare(struct DeviceDescriptor *d, const struct Declaration *decl)
{
  bool result = false;

  assert(d != NULL);

  if (is_simulator())
    return true;

  mutexComm.Lock();
  if (d != NULL)
    d->Declare(decl);
  mutexComm.Unlock();

  return result;
}

bool
devIsLogger(const struct DeviceDescriptor *d)
{
  bool result = false;

  assert(d != NULL);

  mutexComm.Lock();
  if (d != NULL)
    result = d->IsLogger();
  mutexComm.Unlock();

  return result;
}

bool
devIsGPSSource(const struct DeviceDescriptor *d)
{
  bool result = false;

  assert(d != NULL);

  mutexComm.Lock();
  if (d != NULL)
    result = d->IsGPSSource();
  mutexComm.Unlock();

  return result;
}

bool
devIsBaroSource(const struct DeviceDescriptor *d)
{
  bool result = false;

  assert(d != NULL);

  mutexComm.Lock();
  if (d != NULL)
    result = d->IsBaroSource();
  mutexComm.Unlock();

  return result;
}

bool
devIsRadio(const struct DeviceDescriptor *d)
{
  bool result = false;

  assert(d != NULL);

  mutexComm.Lock();
  if (d && d->Driver) {
    result = (d->Driver->Flags & drfRadio) != 0;
  }
  mutexComm.Unlock();

  return result;
}

bool
devIsCondor(const struct DeviceDescriptor *d)
{
  bool result = false;

  assert(d != NULL);

  mutexComm.Lock();
  if (d && d->Driver) {
    result = (d->Driver->Flags & drfCondor) != 0;
  }
  mutexComm.Unlock();

  return result;
}

static bool
devOpenLog(struct DeviceDescriptor *d, const TCHAR *FileName)
{
  assert(d != NULL);

  d->fhLogFile = _tfopen(FileName, _T("a+b"));
  return d->fhLogFile != NULL;
}

static bool
devCloseLog(struct DeviceDescriptor *d)
{
  assert(d != NULL);

  if (d->fhLogFile != NULL){
    fclose(d->fhLogFile);
    return true;
  } else
    return false;
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
  _sntprintf(dst, sz, _T("$%s*%02X\r\n"), text, NMEAChecksum(text));
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
    if (_tcscmp(DeviceList[i].Name, _T("Vega")) == 0)
      if (DeviceList[i].Com)
        DeviceList[i].Com->WriteString(tmp);
  mutexComm.Unlock();
}

struct DeviceDescriptor *devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, _T("Vega")) == 0)
      return &DeviceList[i];
  return NULL;
}

void AllDevicesPutMacCready(double MacCready)
{
  if (is_simulator())
    return;

  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutMacCready(MacCready);
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
  StartupStore(_T("Register serial devices\n"));

  devInit(lpCmdLine);
}

void devShutdown()
{
  int i;

  // Stop COM devices
  StartupStore(_T("Stop COM devices\n"));

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
  StartupStore(_T("RestartCommPorts\n"));

  mutexComm.Lock();

  devShutdown();
  NMEAParser::Reset();

  devInit(_T(""));

  mutexComm.Unlock();
}

void devConnectionMonitor()
{
  mutexComm.Lock();
  NMEAParser::UpdateMonitor();
  mutexComm.Unlock();
}
