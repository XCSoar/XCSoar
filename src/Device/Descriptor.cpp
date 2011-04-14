/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Device/Descriptor.hpp"
#include "Device/Driver.hpp"
#include "Device/Parser.hpp"
#include "Device/FLARM.hpp"
#include "DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "NMEA/Info.hpp"
#include "Thread/Mutex.hpp"
#include "StringUtil.hpp"
#include "Logger/NMEALogger.hpp"
#include "Operation.hpp"
#include "OS/Clock.hpp"

#ifdef ANDROID
#include "Android/InternalGPS.hpp"
#endif

#include <assert.h>

DeviceDescriptor::DeviceDescriptor()
  :Com(NULL), pDevPipeTo(NULL),
   Driver(NULL), device(NULL),
#ifdef ANDROID
   internal_gps(NULL),
#endif
   ticker(false), declaring(false)
{
}

DeviceDescriptor::~DeviceDescriptor()
{
  assert(!declaring);
}

bool
DeviceDescriptor::Open(Port *_port, const struct DeviceRegister *_driver)
{
  assert(_port != NULL);
  assert(_driver != NULL);
  assert(Com == NULL);
  assert(device == NULL);
  assert(!ticker);

  mutexBlackboard.Lock();
  device_blackboard.SetRealState(index).reset();
  device_blackboard.Merge();
  mutexBlackboard.Unlock();

  Com = _port;
  Driver = _driver;

  assert(Driver->CreateOnPort != NULL || (Driver->Flags & drfNmeaOut));
  if (Driver->CreateOnPort == NULL)
    return true;

  parser.Reset();
  parser.SetReal(_tcscmp(Driver->Name, _T("Condor")) != 0);

  device = Driver->CreateOnPort(Com);
  OperationEnvironment env;
  if (!device->Open(env)) {
    delete device;
    device = NULL;
    Com = NULL;
    return false;
  }

  return true;
}

void
DeviceDescriptor::Close()
{
#ifdef ANDROID
  delete internal_gps;
  internal_gps = NULL;
#endif

  delete device;
  device = NULL;

  Port *OldCom = Com;
  Com = NULL;

  delete OldCom;

  Driver = NULL;
  pDevPipeTo = NULL;
  ticker = false;

  mutexBlackboard.Lock();
  device_blackboard.SetRealState(index).reset();
  device_blackboard.Merge();
  mutexBlackboard.Unlock();
}

const TCHAR *
DeviceDescriptor::GetName() const
{
  return Driver != NULL ? Driver->Name : NULL;
}

bool
DeviceDescriptor::IsDriver(const TCHAR *name) const
{
  return Driver != NULL
    ? _tcscmp(Driver->Name, name) == 0
    : false;
}

bool
DeviceDescriptor::IsLogger() const
{
  return Driver != NULL &&
    ((Driver->Flags & drfLogger) != 0 ||
     (device != NULL && device->IsLogger()) ||
     parser.isFlarm);
}

bool
DeviceDescriptor::IsBaroSource() const
{
  return Driver != NULL &&
    ((Driver->Flags & drfBaroAlt) != 0 ||
     (device != NULL && device->IsBaroSource()));
}

bool
DeviceDescriptor::IsRadio() const
{
  return Driver != NULL && (Driver->Flags & drfRadio) != 0;
}

bool
DeviceDescriptor::ParseNMEA(const char *String, NMEA_INFO *GPS_INFO)
{
  assert(String != NULL);
  assert(GPS_INFO != NULL);

  if (device != NULL && device->ParseNMEA(String, GPS_INFO)) {
    GPS_INFO->Connected.update(fixed(MonotonicClockMS()) / 1000);
    return true;
  }

  // Additional "if" to find GPS strings
  if (parser.ParseNMEAString_Internal(String, GPS_INFO)) {
    GPS_INFO->Connected.update(fixed(MonotonicClockMS()) / 1000);
    return true;
  }

  return false;
}

bool
DeviceDescriptor::PutMacCready(fixed MacCready)
{
  return device != NULL ? device->PutMacCready(MacCready) : true;
}

bool
DeviceDescriptor::PutBugs(fixed bugs)
{
  return device != NULL ? device->PutBugs(bugs) : true;
}

bool
DeviceDescriptor::PutBallast(fixed ballast)
{
  return device != NULL ? device->PutBallast(ballast) : true;
}

bool
DeviceDescriptor::PutVolume(int volume)
{
  return device != NULL ? device->PutVolume(volume) : true;
}

bool
DeviceDescriptor::PutActiveFrequency(double frequency)
{
  return device != NULL ? device->PutActiveFrequency(frequency) : true;
}

bool
DeviceDescriptor::PutStandbyFrequency(double frequency)
{
  return device != NULL ? device->PutStandbyFrequency(frequency) : true;
}

bool
DeviceDescriptor::PutQNH(const AtmosphericPressure &pres,
                         const DERIVED_INFO &calculated)
{
  return device != NULL ? device->PutQNH(pres, calculated) : true;
}

bool
DeviceDescriptor::PutVoice(const TCHAR *sentence)
{
  return device != NULL ? device->PutVoice(sentence) : true;
}

void
DeviceDescriptor::LinkTimeout()
{
  assert(!declaring);

  if (device != NULL)
    device->LinkTimeout();
}

bool
DeviceDescriptor::Declare(const struct Declaration *declaration)
{
  assert(!declaring);
  declaring = true;

  OperationEnvironment env;
  bool result = device != NULL && device->Declare(declaration, env);

  if (parser.isFlarm)
    result = FlarmDeclare(Com, declaration) || result;

  declaring = false;
  return result;
}

void
DeviceDescriptor::OnSysTicker(const NMEA_INFO &basic,
                              const DERIVED_INFO &calculated)
{
  if (device == NULL || declaring)
    return;

  ScopeLock protect(mutexBlackboard);

  ticker = !ticker;
  if (ticker)
    // write settings to vario every second
    device->OnSysTicker(basic, calculated);
}

void
DeviceDescriptor::LineReceived(const char *line)
{
  LogNMEA(line);

  if (pDevPipeTo && pDevPipeTo->Com) {
    // stream pipe, pass nmea to other device (NmeaOut)
    // TODO code: check TX buffer usage and skip it if buffer is full (outbaudrate < inbaudrate)
    pDevPipeTo->Com->Write(line);
    pDevPipeTo->Com->Write("\r\n");
  }

  ScopeLock protect(mutexBlackboard);
  if (ParseNMEA(line, &device_blackboard.SetRealState(index)))
    device_blackboard.Merge();
}
