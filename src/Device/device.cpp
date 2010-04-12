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
#include "NMEA/Checksum.h"
#include "options.h" /* for LOGGDEVCOMMANDLINE */
#include "Asset.hpp"
#include "Simulator.hpp"
#include "Profile.hpp"

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

/**
 * Attempt to detect the GPS device.
 *
 * See http://msdn.microsoft.com/en-us/library/bb202042.aspx
 */
static bool
detect_gps(TCHAR *path, size_t path_max_size)
{
#ifdef _WIN32_WCE
  static const TCHAR *const gps_idm_key =
    _T("System\\CurrentControlSet\\GPS Intermediate Driver\\Multiplexer");
  static const TCHAR *const gps_idm_value = _T("DriverInterface");

  HKEY hKey;
  long result;

  result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, gps_idm_key, 0, KEY_READ, &hKey);
  if (result != ERROR_SUCCESS)
    return false;

  DWORD type, size = path_max_size;
  result = RegQueryValueEx(hKey, gps_idm_value, 0, &type, (LPBYTE)path, &size);
  RegCloseKey(hKey);

  return result == ERROR_SUCCESS && type == REG_SZ;
#else
  return false;
#endif
}

static bool
devInitOne(DeviceDescriptor &device, const DeviceConfig &config,
           DeviceDescriptor *&nmeaout)
{
  if (is_simulator())
    return false;

  const struct DeviceRegister *Driver = devGetDriver(config.driver_name);
  if (Driver == NULL)
    return false;

  const TCHAR *path = NULL;
  TCHAR buffer[MAX_PATH];

  switch (config.port_type) {
  case DeviceConfig::SERIAL:
    path = COMMPort[config.port_index];
    break;

  case DeviceConfig::AUTO:
    if (!detect_gps(buffer, sizeof(buffer))) {
      LogStartUp(_T("no GPS detected"));
      return false;
    }

    LogStartUp(_T("GPS detected: %s"), buffer);

    path = buffer;
    break;
  }

  if (path == NULL)
    return false;

  ComPort *Com = new ComPort(path, dwSpeed[config.speed_index],
                             device);
  if (!Com->Open()) {
    delete Com;
    return false;
  }

  device.Driver = Driver;
  device.Com = Com;
  device.Open();

  device.enable_baro = devIsBaroSource(device) && !devHasBaroSource();

  if (nmeaout == NULL && Driver->Flags & (1l << dfNmeaOut))
    nmeaout = &device;

  return true;
}

static void
ParseLogOption(DeviceDescriptor &device, const TCHAR *CommandLine,
               const TCHAR *option)
{
  assert(CommandLine != NULL);
  assert(option != NULL);

  const TCHAR *start = _tcsstr(CommandLine, option);
  if (start == NULL)
    return;

  start += _tcslen(option);

  const TCHAR *end;
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
              device.GetName(), path);
    MessageBoxX(msg, gettext(_T("Information")), MB_OK | MB_ICONINFORMATION);
  } else {
    TCHAR msg[512];
    _stprintf(msg, _T("Unable to open log\r\non device %s\r\n%s"),
              device.GetName(), path);
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
  struct DeviceDescriptor *pDevNmeaOut = NULL;

  for (unsigned i = 0; i < NUMDEV; i++)
    DeviceList[i].Clear();

  DeviceConfig config[NUMDEV];
  if (is_altair()) {
    config[0].port_index = 2;
    config[0].speed_index = 5;
    config[1].port_index = 0;
    config[1].speed_index = 5;
  } else {
    config[0].port_index = 0;
    config[0].speed_index = 2;
    config[1].port_index = 0;
    config[1].speed_index = 2;
  }

  for (unsigned i = 0; i < NUMDEV; ++i)
    Profile::GetDeviceConfig(i, config[i]);

  devInitOne(DeviceList[0], config[0], pDevNmeaOut);

  if (config[0].port_index != config[1].port_index)
    devInitOne(DeviceList[1], config[1], pDevNmeaOut);

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
devDeclare(DeviceDescriptor &d, const struct Declaration *decl)
{
  if (is_simulator())
    return true;

  ScopeLock protect(mutexComm);
  return d.Declare(decl);
}

bool
devIsLogger(const DeviceDescriptor &d)
{
  ScopeLock protect(mutexComm);
  return d.IsLogger();
}

bool
devIsGPSSource(const DeviceDescriptor &d)
{
  ScopeLock protect(mutexComm);
  return d.IsGPSSource();
}

bool
devIsBaroSource(const DeviceDescriptor &d)
{
  ScopeLock protect(mutexComm);
  return d.IsBaroSource();
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
devWriteNMEAString(DeviceDescriptor &d, const TCHAR *text)
{
  TCHAR tmp[512];

  if (d.Com == NULL)
    return;

  devFormatNMEAString(tmp, 512, text);

  ScopeLock protect(mutexComm);
  d.Com->WriteString(tmp);
}

void
VarioWriteNMEA(const TCHAR *text)
{
  TCHAR tmp[512];

  devFormatNMEAString(tmp, 512, text);

  ScopeLock protect(mutexComm);
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      if (DeviceList[i].Com)
        DeviceList[i].Com->WriteString(tmp);
}

struct DeviceDescriptor *
devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      return &DeviceList[i];

  return NULL;
}

void
devStartup(const TCHAR *lpCmdLine)
{
  LogStartUp(_T("Register serial devices"));

  devInit(lpCmdLine);
}

void
devShutdown()
{
  int i;

  // Stop COM devices
  LogStartUp(_T("Stop COM devices"));

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

  LogStartUp(_T("RestartCommPorts"));

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
