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

#include "Device/device.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Parser.hpp"
#include "Device/Port.hpp"
#include "Thread/Mutex.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Registry.hpp"
#include "NMEA/Checksum.h"
#include "options.h" /* for LOGGDEVCOMMANDLINE */
#include "Asset.hpp"

#include <assert.h>

Mutex mutexComm;

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

static const DWORD dwSpeed[] = {
  1200,
  2400,
  4800,
  9600,
  19200,
  38400,
  57600,
  115200
};

// This function is used to determine whether a generic
// baro source needs to be used if available
bool
devHasBaroSource(void)
{
  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceList[i].enable_baro)
      return true;

  return false;
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
    ComPort *Com = new ComPort(port, speed, *dev);

    if (!Com->Open())
      return false;

    memset(dev->Name, 0, sizeof(dev->Name));
    _tcsncpy(dev->Name, Driver->Name, DEVNAMESIZE);

    dev->Driver = Driver;

    dev->Com = Com;

    dev->Open(index);

    dev->enable_baro = devIsBaroSource(dev) && !devHasBaroSource();

    if (nmeaout == NULL && Driver->Flags & (1l << dfNmeaOut)) {
      nmeaout = dev;
    }
  }

  return true;
}

static void
ParseLogOption(DeviceDescriptor &device, const TCHAR *CommandLine,
               const TCHAR *option)
{
  assert(CommandLine != NULL);
  assert(option != NULL);

  TCHAR *start = _tcsstr(CommandLine, option);
  if (start == NULL)
    return;

  start += _tcslen(option);

  TCHAR *end;
  if (*start == '"') {
    start++;

    end = _tcschr(start, _T('"'));
    if (end == NULL)
      /* ignoring the parser error (missing double quote) */
      return;
  } else {
    end = _tcschr(start, _T(' '));
    if (end == NULL)
      end = start + _tcslen(start);
  }

  if (start >= end)
    return;

  TCHAR path[MAX_PATH];
  _tcsncpy(path, start, end - start);
  path[end - start] = '\0';

  if (device.OpenLog(path)) {
    TCHAR msg[512];
    _stprintf(msg, _T("Device %s logs to\r\n%s"),
              device.Name, path);
    MessageBoxX(msg, gettext(_T("Information")), MB_OK | MB_ICONINFORMATION);
  } else {
    TCHAR msg[512];
    _stprintf(msg, _T("Unable to open log\r\non device %s\r\n%s"),
              device.Name, path);
    MessageBoxX(msg, gettext(_T("Error")), MB_OK | MB_ICONWARNING);
  }
}

static void
SetPipeTo(DeviceDescriptor &out)
{
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDescriptor &device = DeviceList[i];

    device.pDevPipeTo = &device == &out ? NULL : &out;
  }
}

static bool
devInit(const TCHAR *CommandLine)
{
  int i;
  struct DeviceDescriptor *pDevNmeaOut = NULL;

  for (i = 0; i < NUMDEV; i++) {
    DeviceList[i].Port = -1;
    DeviceList[i].fhLogFile = NULL;
    DeviceList[i].Name[0] = '\0';
    DeviceList[i].Driver = NULL;
    DeviceList[i].pDevPipeTo = NULL;
    DeviceList[i].enable_baro = false;
  }

  DWORD PortIndex1, PortIndex2, SpeedIndex1, SpeedIndex2;
  if (is_altair()) {
    PortIndex1 = 2;
    SpeedIndex1 = 5;
    PortIndex2 = 0;
    SpeedIndex2 = 5;
  } else {
    PortIndex1 = 0;
    SpeedIndex1 = 2;
    PortIndex2 = 0;
    SpeedIndex2 = 2;
  }

  ReadPort1Settings(&PortIndex1, &SpeedIndex1);
  ReadPort2Settings(&PortIndex2, &SpeedIndex2);

  devInitOne(&DeviceList[0], 0, COMMPort[PortIndex1], dwSpeed[SpeedIndex1], pDevNmeaOut);

  if (PortIndex1 != PortIndex2)
    devInitOne(&DeviceList[0], 1, COMMPort[PortIndex2], dwSpeed[SpeedIndex2], pDevNmeaOut);

  CommandLine = LOGGDEVCOMMANDLINE;

  if (CommandLine != NULL) {
    ParseLogOption(DeviceList[0], CommandLine, _T("-logA="));
    ParseLogOption(DeviceList[1], CommandLine, _T("-logB="));
  }

  if (pDevNmeaOut != NULL)
    SetPipeTo(*pDevNmeaOut);

  return true;
}

bool
devDeclare(struct DeviceDescriptor *d, const struct Declaration *decl)
{
  assert(d != NULL);

  if (is_simulator())
    return true;

  ScopeLock protect(mutexComm);
  return d->Declare(decl);
}

bool
devIsLogger(const struct DeviceDescriptor *d)
{
  assert(d != NULL);

  ScopeLock protect(mutexComm);
  return d->IsLogger();
}

bool
devIsGPSSource(const struct DeviceDescriptor *d)
{
  assert(d != NULL);

  ScopeLock protect(mutexComm);
  return d->IsGPSSource();
}

bool
devIsBaroSource(const struct DeviceDescriptor *d)
{
  assert(d != NULL);

  ScopeLock protect(mutexComm);
  return d->IsBaroSource();
}

bool
HaveCondorDevice()
{
  ScopeLock protect(mutexComm);

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceList[i].IsCondor())
      return true;

  return false;
}

static void
devFormatNMEAString(TCHAR *dst, size_t sz, const TCHAR *text)
{
  _sntprintf(dst, sz, _T("$%s*%02X\r\n"), text, NMEAChecksum(text));
}

void
devWriteNMEAString(struct DeviceDescriptor *d, const TCHAR *text)
{
  TCHAR tmp[512];

  assert(d != NULL);

  if (d->Com == NULL)
    return;

  devFormatNMEAString(tmp, 512, text);

  ScopeLock protect(mutexComm);
  d->Com->WriteString(tmp);
}

void
VarioWriteNMEA(const TCHAR *text)
{
  TCHAR tmp[512];

  devFormatNMEAString(tmp, 512, text);

  ScopeLock protect(mutexComm);
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, _T("Vega")) == 0)
      if (DeviceList[i].Com)
        DeviceList[i].Com->WriteString(tmp);
}

struct DeviceDescriptor *
devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, _T("Vega")) == 0)
      return &DeviceList[i];

  return NULL;
}

void
devStartup(const TCHAR *lpCmdLine)
{
  StartupStore(_T("Register serial devices\n"));

  devInit(lpCmdLine);
}

void
devShutdown()
{
  int i;

  // Stop COM devices
  StartupStore(_T("Stop COM devices\n"));

  for (i = 0; i < NUMDEV; i++) {
    DeviceList[i].Close();
    DeviceList[i].CloseLog();
  }
}

void
devRestart()
{
  if (is_simulator())
    return;

  /*
  #ifdef WINDOWSPC
  static bool first = true;
  if (!first) {
    return;
  }
  first = false;
  #endif
  */

  StartupStore(_T("RestartCommPorts\n"));

  ScopeLock protect(mutexComm);

  devShutdown();

  devInit(_T(""));
}

void devConnectionMonitor()
{
  ScopeLock protect(mutexComm);

  /* check which port has valid GPS information and activate it */

  bool active = false;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    if (!active && DeviceList[i].parser.gpsValid) {
      DeviceList[i].parser.activeGPS = true;
      active = true;
    } else
      DeviceList[i].parser.activeGPS = false;
  }

  if (!active)
    /* none - activate first device anyway */
    DeviceList[0].parser.activeGPS = true;
}
