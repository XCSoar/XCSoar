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
#include "Operation/Operation.hpp"
#include "OS/Clock.hpp"
#include "../Simulator.hpp"
#include "Asset.hpp"
#include "Input/InputQueue.hpp"
#include "LogFile.hpp"

#ifdef ANDROID
#include "Java/Object.hpp"
#include "Java/Global.hpp"
#include "Android/InternalGPS.hpp"
#include "Android/Main.hpp"
#endif

#include <assert.h>

DeviceDescriptor::DeviceDescriptor()
  :port(NULL), monitor(NULL),
   pipe_to_device(NULL),
   driver(NULL), device(NULL),
#ifdef ANDROID
   internal_gps(NULL),
#endif
   ticker(false), busy(false)
{
}

bool
DeviceDescriptor::Open(Port &_port, const DeviceRegister &_driver,
                       OperationEnvironment &env)
{
  assert(port == NULL);
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

  port = &_port;
  driver = &_driver;

  assert(driver->CreateOnPort != NULL || driver->IsNMEAOut());
  if (driver->CreateOnPort == NULL)
    return true;

  parser.Reset();
  parser.SetReal(_tcscmp(driver->name, _T("Condor")) != 0);
  if (config.IsDriver(_T("Condor")))
    parser.DisableGeoid();

  device = driver->CreateOnPort(config, *port);
  if (!device->Open(env) || !port->StartRxThread()) {
    delete device;
    device = NULL;
    port = NULL;
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
  TCHAR buffer[64];
  LogStartUp(_T("Open device %s"), config.GetPortName(buffer, 64));

  if (config.port_type == DeviceConfig::PortType::INTERNAL)
    return OpenInternalGPS();

  const struct DeviceRegister *driver = FindDriverByName(config.driver_name);
  if (driver == NULL) {
    StaticString<256> msg;
    msg.Format(_T("%s: %s."), _("No such driver"), config.driver_name.c_str());
    env.SetErrorMessage(msg);
    return false;
  }

  Port *port = OpenPort(config, *this);
  if (port == NULL) {
    TCHAR name_buffer[64];
    const TCHAR *name = config.GetPortName(name_buffer, 64);

    StaticString<256> msg;
    msg.Format(_T("%s: %s."), _("Unable to open port"), name);
    env.SetErrorMessage(msg);
    return false;
  }

  if (!Open(*port, *driver, env)) {
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

  Port *old_port = port;
  port = NULL;
  delete old_port;

  driver = NULL;
  pipe_to_device = NULL;
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
  if (IsAltair() || !config.IsAvailable() || IsConnected() ||
      (driver != NULL && !driver->HasTimeout()) ||
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
  return driver != NULL ? driver->display_name : NULL;
}

bool
DeviceDescriptor::IsDriver(const TCHAR *name) const
{
  return driver != NULL
    ? _tcscmp(driver->name, name) == 0
    : false;
}

bool
DeviceDescriptor::CanDeclare() const
{
  return driver != NULL &&
    (driver->CanDeclare() ||
     device_blackboard->IsFLARM(index));
}

bool
DeviceDescriptor::IsLogger() const
{
  return driver != NULL && driver->IsLogger();
}

bool
DeviceDescriptor::IsNMEAOut() const
{
  return driver != NULL && driver->IsNMEAOut();
}

bool
DeviceDescriptor::IsManageable() const
{
  return driver != NULL && driver->IsManageable();
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

  if (port != NULL)
    PortWriteNMEA(*port, line);
}

#ifdef _UNICODE
void
DeviceDescriptor::WriteNMEA(const TCHAR *line)
{
  assert(line != NULL);

  if (port == NULL)
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
DeviceDescriptor::Declare(const struct Declaration &declaration,
                          const Waypoint *home,
                          OperationEnvironment &env)
{
  if (port == NULL)
    return false;

  SetBusy(true);

  StaticString<60> text;
  text.Format(_T("%s: %s."), _("Sending declaration"), driver->display_name);
  env.SetText(text);

  port->StopRxThread();

  bool result = device != NULL && device->Declare(declaration, home, env);

  if (device_blackboard->IsFLARM(index) && !IsDriver(_T("FLARM"))) {
    text.Format(_T("%s: FLARM."), _("Sending declaration"));
    env.SetText(text);
    FlarmDevice flarm(*port);
    result = flarm.Declare(declaration, home, env) || result;
  }

  port->StartRxThread();

  SetBusy(false);
  return result;
}

bool
DeviceDescriptor::EnableDownloadMode()
{
  if (port == NULL || device == NULL)
    return false;

  port->StopRxThread();
  bool result = device->EnableDownloadMode();
  port->StartRxThread();
  return result;
}

bool
DeviceDescriptor::DisableDownloadMode()
{
  if (port == NULL || device == NULL)
    return false;

  port->StopRxThread();
  bool result = device->DisableDownloadMode();
  port->StartRxThread();
  return result;
}

bool
DeviceDescriptor::ReadFlightList(RecordedFlightList &flight_list,
                                 OperationEnvironment &env)
{
  if (port == NULL || driver == NULL || device == NULL)
    return false;

  StaticString<60> text;
  text.Format(_T("%s: %s."), _("Reading flight list"), driver->display_name);
  env.SetText(text);

  port->StopRxThread();
  bool result = device->ReadFlightList(flight_list, env);
  port->StartRxThread();
  return result;
}

bool
DeviceDescriptor::DownloadFlight(const RecordedFlightInfo &flight,
                                 const TCHAR *path,
                                 OperationEnvironment &env)
{
  if (port == NULL || driver == NULL || device == NULL)
    return false;

  StaticString<60> text;
  text.Format(_T("%s: %s."), _("Downloading flight log"), driver->display_name);
  env.SetText(text);

  port->StopRxThread();
  bool result = device->DownloadFlight(flight, path, env);
  port->StartRxThread();
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

  if (pipe_to_device && pipe_to_device->port) {
    // stream pipe, pass nmea to other device (NmeaOut)
    // TODO code: check TX buffer usage and skip it if buffer is full (outbaudrate < inbaudrate)
    pipe_to_device->port->Write(line);
    pipe_to_device->port->Write("\r\n");
  }

  if (ParseLine(line))
    device_blackboard->ScheduleMerge();
}
