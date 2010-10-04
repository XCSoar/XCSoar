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
#include "Device/Internal.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Parser.hpp"
#include "Device/Port.hpp"
#include "Device/SerialPort.hpp"
#include "Device/NullPort.hpp"
#include "Thread/Mutex.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Asset.hpp"
#include "Simulator.hpp"
#include "Profile/Profile.hpp"

#ifdef _WIN32_WCE
#include "Config/Registry.hpp"
#endif

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

  RegistryKey key(HKEY_LOCAL_MACHINE, gps_idm_key, true);
  return !key.error() &&
    key.get_value(gps_idm_value, path, path_max_size);
#else
  return false;
#endif
}

static Port *
OpenPort(const DeviceConfig &config, Port::Handler &handler)
{
  if (is_simulator())
    return new NullPort(handler);

  const TCHAR *path = NULL;
  TCHAR buffer[MAX_PATH];

  switch (config.port_type) {
  case DeviceConfig::SERIAL:
    path = COMMPort[config.port_index];
    break;

  case DeviceConfig::AUTO:
    if (!detect_gps(buffer, sizeof(buffer))) {
      LogStartUp(_T("no GPS detected"));
      return NULL;
    }

    LogStartUp(_T("GPS detected: %s"), buffer);

    path = buffer;
    break;
  }

  if (path == NULL)
    return NULL;

  SerialPort *Com = new SerialPort(path, dwSpeed[config.speed_index], handler);
  if (!Com->Open()) {
    delete Com;
    return NULL;
  }

  return Com;
}

static bool
devInitOne(DeviceDescriptor &device, const DeviceConfig &config,
           DeviceDescriptor *&nmeaout)
{
  const struct DeviceRegister *Driver = devGetDriver(config.driver_name);
  if (Driver == NULL)
    return false;

  Port *Com = OpenPort(config, device);
  if (Com == NULL)
    return false;

  device.Driver = Driver;
  device.Com = Com;
  device.Open();

  device.enable_baro = devIsBaroSource(device) && !devHasBaroSource();

  if (nmeaout == NULL && Driver->Flags & (1l << dfNmeaOut))
    nmeaout = &device;

  return true;
}

static void
SetPipeTo(DeviceDescriptor &out)
{
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDescriptor &device = DeviceList[i];

    device.pDevPipeTo = &device == &out ? NULL : &out;
  }
}

void
devStartup()
{
  LogStartUp(_T("Register serial devices"));

  DeviceDescriptor *pDevNmeaOut = NULL;

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

  Profile::Get(szProfileIgnoreNMEAChecksum, NMEAParser::ignore_checksum);

  for (unsigned i = 0; i < NUMDEV; ++i)
    Profile::GetDeviceConfig(i, config[i]);

  devInitOne(DeviceList[0], config[0], pDevNmeaOut);

  if (config[0].port_index != config[1].port_index)
    devInitOne(DeviceList[1], config[1], pDevNmeaOut);

  if (pDevNmeaOut != NULL)
    SetPipeTo(*pDevNmeaOut);
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

#ifdef _UNICODE
static void
PortWriteNMEA(Port *port, const TCHAR *line)
{
  assert(port != NULL);
  assert(line != NULL);

  char buffer[_tcslen(line) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, line, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return;

  PortWriteNMEA(port, buffer);
}
#endif

void
devWriteNMEAString(DeviceDescriptor &d, const TCHAR *text)
{
  if (d.Com == NULL)
    return;

  ScopeLock protect(mutexComm);
  PortWriteNMEA(d.Com, text);
}

void
VarioWriteNMEA(const TCHAR *text)
{
  ScopeLock protect(mutexComm);
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      if (DeviceList[i].Com)
        PortWriteNMEA(DeviceList[i].Com, text);
}

DeviceDescriptor *
devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      return &DeviceList[i];

  return NULL;
}

void
devShutdown()
{
  int i;

  // Stop COM devices
  LogStartUp(_T("Stop COM devices"));

  for (i = 0; i < NUMDEV; i++) {
    DeviceList[i].Close();
  }
}

void
devRestart()
{
  LogStartUp(_T("RestartCommPorts"));

  ScopeLock protect(mutexComm);

  devShutdown();

  devStartup();
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
