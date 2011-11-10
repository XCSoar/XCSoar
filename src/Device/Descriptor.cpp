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
#include "Driver/FLARM/Device.hpp"
#include "Device/Internal.hpp"
#include "Device/Register.hpp"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Port/ConfiguredPort.hpp"
#include "NMEA/Info.hpp"
#include "Thread/Mutex.hpp"
#include "StringUtil.hpp"
#include "Logger/NMEALogger.hpp"
#include "Language/Language.hpp"
#include "Operation.hpp"
#include "OS/Clock.hpp"
#include "../Simulator.hpp"
#include "Asset.hpp"
#include "InputEvents.hpp"
#include "LogFile.hpp"

#ifdef ANDROID
#include "Java/Object.hpp"
#include "Java/Global.hpp"
#include "Android/InternalGPS.hpp"
#include "Android/Main.hpp"
#endif

#include <assert.h>

DeviceDescriptor::DeviceDescriptor()
  :Com(NULL), monitor(NULL),
   pDevPipeTo(NULL),
   Driver(NULL), device(NULL),
#ifdef ANDROID
   internal_gps(NULL),
#endif
   ticker(false), busy(false)
{
}

DeviceDescriptor::~DeviceDescriptor()
{
  assert(!busy);
}

bool
DeviceDescriptor::Open(Port *_port, const struct DeviceRegister *_driver,
                       OperationEnvironment &env)
{
  assert(_port != NULL);
  assert(_driver != NULL);
  assert(Com == NULL);
  assert(device == NULL);
  assert(!ticker);

  reopen_clock.update();

  device_blackboard->mutex.Lock();
  device_blackboard->SetRealState(index).Reset();
  device_blackboard->ScheduleMerge();
  device_blackboard->mutex.Unlock();

  settings_sent.Clear();
  settings_received.Clear();
  was_connected = false;

  Com = _port;
  Driver = _driver;

  assert(Driver->CreateOnPort != NULL || Driver->IsNMEAOut());
  if (Driver->CreateOnPort == NULL)
    return true;

  parser.Reset();
  parser.SetReal(_tcscmp(Driver->name, _T("Condor")) != 0);
  if (config.IsDriver(_T("Condor")))
    parser.DisableGeoid();

  device = Driver->CreateOnPort(config, Com);
  if (!device->Open(env) || !Com->StartRxThread()) {
    delete device;
    device = NULL;
    Com = NULL;
    return false;
  }

  return true;
}

bool
DeviceDescriptor::OpenInternalGPS()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  internal_gps = InternalGPS::create(Java::GetEnv(), context, GetIndex());
  return internal_gps != NULL;
#else
  return false;
#endif
}

bool
DeviceDescriptor::Open(OperationEnvironment &env)
{
  if (config.port_type == DeviceConfig::PortType::INTERNAL)
    return OpenInternalGPS();

  const struct DeviceRegister *driver = FindDriverByName(config.driver_name);
  if (driver == NULL) {
    TCHAR msg[256];
    _sntprintf(msg, 256, _T("%s: %s."), _("No such driver"), config.driver_name.c_str());
    env.SetErrorMessage(msg);
    return false;
  }

  Port *port = OpenPort(config, *this);
  if (port == NULL) {
    TCHAR name_buffer[64];
    const TCHAR *name = config.GetPortName(name_buffer, 64);

    TCHAR msg[256];
    _sntprintf(msg, 256, _T("%s: %s."), _("Unable to open port"), name);
    env.SetErrorMessage(msg);
    return false;
  }

  if (!Open(port, driver, env)) {
    delete port;
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

  device_blackboard->mutex.Lock();
  device_blackboard->SetRealState(index).Reset();
  device_blackboard->ScheduleMerge();
  device_blackboard->mutex.Unlock();

  settings_sent.Clear();
  settings_received.Clear();
}

bool
DeviceDescriptor::Reopen(OperationEnvironment &env)
{
  Close();
  return Open(env);
}

void
DeviceDescriptor::AutoReopen(OperationEnvironment &env)
{
  if (is_altair() || !config.IsAvailable() || IsConnected() ||
      (Driver != NULL && !Driver->HasTimeout()) ||
      /* attempt to reopen a failed device every 30 seconds */
      !reopen_clock.check_update(30000))
    return;

  TCHAR buffer[64];
  LogStartUp(_T("Reconnecting to device %s"), config.GetPortName(buffer, 64));

  InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
  Reopen(env);
}

const TCHAR *
DeviceDescriptor::GetDisplayName() const
{
  return Driver != NULL ? Driver->display_name : NULL;
}

bool
DeviceDescriptor::IsDriver(const TCHAR *name) const
{
  return Driver != NULL
    ? _tcscmp(Driver->name, name) == 0
    : false;
}

bool
DeviceDescriptor::CanDeclare() const
{
  return Driver != NULL &&
    (Driver->CanDeclare() ||
     device_blackboard->IsFLARM(index));
}

bool
DeviceDescriptor::IsLogger() const
{
  return Driver != NULL && Driver->IsLogger();
}

bool
DeviceDescriptor::IsNMEAOut() const
{
  return Driver != NULL && Driver->IsNMEAOut();
}

bool
DeviceDescriptor::IsManageable() const
{
  return Driver != NULL && Driver->IsManageable();
}

bool
DeviceDescriptor::IsConnected() const
{
  ScopeLock protect(device_blackboard->mutex);
  return device_blackboard->RealState(index).connected;
}

bool
DeviceDescriptor::ParseNMEA(const char *line, NMEAInfo &info)
{
  assert(line != NULL);

  /* restore the driver's ExternalSettings */
  const ExternalSettings old_settings = info.settings;
  info.settings = settings_received;

  if (device != NULL && device->ParseNMEA(line, info)) {
    info.connected.Update(fixed(MonotonicClockMS()) / 1000);

    /* clear the settings when the values are the same that we already
       sent to the device */
    const ExternalSettings old_received = settings_received;
    settings_received = info.settings;
    info.settings.EliminateRedundant(settings_sent, old_received);

    return true;
  }

  /* no change - restore the ExternalSettings that we returned last
     time */
  info.settings = old_settings;

  // Additional "if" to find GPS strings
  if (parser.ParseLine(line, info)) {
    info.connected.Update(fixed(MonotonicClockMS()) / 1000);
    return true;
  }

  return false;
}

void
DeviceDescriptor::WriteNMEA(const char *line)
{
  assert(line != NULL);

  if (Com != NULL)
    PortWriteNMEA(Com, line);
}

#ifdef _UNICODE
void
DeviceDescriptor::WriteNMEA(const TCHAR *line)
{
  assert(line != NULL);

  if (Com == NULL)
    return;

  char buffer[_tcslen(line) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, line, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return;

  WriteNMEA(buffer);
}
#endif

bool
DeviceDescriptor::PutMacCready(fixed value)
{
  if (device == NULL || settings_sent.CompareMacCready(value))
    return true;

  if (!device->PutMacCready(value))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.mac_cready = value;
  settings_sent.mac_cready_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutBugs(fixed value)
{
  if (device == NULL || settings_sent.CompareBugs(value))
    return true;

  if (!device->PutBugs(value))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.bugs = value;
  settings_sent.bugs_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutBallast(fixed value)
{
  if (device == NULL || settings_sent.CompareBallastFraction(value))
    return true;

  if (!device->PutBallast(value))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.ballast_fraction = value;
  settings_sent.ballast_fraction_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutVolume(int volume)
{
  return device != NULL ? device->PutVolume(volume) : true;
}

bool
DeviceDescriptor::PutActiveFrequency(RadioFrequency frequency)
{
  return device != NULL ? device->PutActiveFrequency(frequency) : true;
}

bool
DeviceDescriptor::PutStandbyFrequency(RadioFrequency frequency)
{
  return device != NULL ? device->PutStandbyFrequency(frequency) : true;
}

bool
DeviceDescriptor::PutQNH(const AtmosphericPressure &value)
{
  if (device == NULL || settings_sent.CompareQNH(value))
    return true;

  if (!device->PutQNH(value))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.qnh = value;
  settings_sent.qnh_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutVoice(const TCHAR *sentence)
{
  return device != NULL ? device->PutVoice(sentence) : true;
}

bool
DeviceDescriptor::Declare(const struct Declaration &declaration,
                          OperationEnvironment &env)
{
  if (Com == NULL)
    return false;

  SetBusy(true);

  TCHAR text[60];

  _stprintf(text, _T("%s: %s."), _("Sending declaration"), Driver->display_name);
  env.SetText(text);

  Com->StopRxThread();

  bool result = device != NULL && device->Declare(declaration, env);

  if (device_blackboard->IsFLARM(index)) {
    _stprintf(text, _T("%s: FLARM."), _("Sending declaration"));
    env.SetText(text);
    FlarmDevice flarm(*Com);
    result = flarm.Declare(declaration, env) || result;
  }

  Com->StartRxThread();

  SetBusy(false);
  return result;
}

bool
DeviceDescriptor::ReadFlightList(RecordedFlightList &flight_list,
                                 OperationEnvironment &env)
{
  if (Com == NULL || Driver == NULL || device == NULL)
    return false;

  TCHAR text[60];
  _stprintf(text, _T("%s: %s."), _("Reading flight list"), Driver->display_name);
  env.SetText(text);

  Com->StopRxThread();
  bool result = device->ReadFlightList(flight_list, env);
  Com->StartRxThread();
  return result;
}

bool
DeviceDescriptor::DownloadFlight(const RecordedFlightInfo &flight,
                                 const TCHAR *path,
                                 OperationEnvironment &env)
{
  if (Driver == NULL || device == NULL)
    return false;

  TCHAR text[60];
  _stprintf(text, _T("%s: %s."), _("Downloading flight log"), Driver->display_name);
  env.SetText(text);

  Com->StopRxThread();
  bool result = device->DownloadFlight(flight, path, env);
  Com->StartRxThread();
  return result;
}

void
DeviceDescriptor::OnSysTicker(const DerivedInfo &calculated)
{
  if (device == NULL || IsBusy())
    return;

  const bool now_connected = IsConnected();
  if (!now_connected && was_connected)
    /* connection was just lost */
    device->LinkTimeout();

  was_connected = now_connected;

  if (now_connected) {
    ticker = !ticker;
    if (ticker)
      // write settings to vario every second
      device->OnSysTicker(calculated);
  }
}

bool
DeviceDescriptor::ParseLine(const char *line)
{
  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  basic.UpdateClock();
  return ParseNMEA(line, basic);
}

void
DeviceDescriptor::DataReceived(const void *data, size_t length)
{
  if (monitor != NULL)
    monitor->DataReceived(data, length);

  PortLineHandler::DataReceived(data, length);
}

void
DeviceDescriptor::LineReceived(const char *line)
{
  NMEALogger::Log(line);

  if (pDevPipeTo && pDevPipeTo->Com) {
    // stream pipe, pass nmea to other device (NmeaOut)
    // TODO code: check TX buffer usage and skip it if buffer is full (outbaudrate < inbaudrate)
    pDevPipeTo->Com->Write(line);
    pDevPipeTo->Com->Write("\r\n");
  }

  if (ParseLine(line))
    device_blackboard->ScheduleMerge();
}
