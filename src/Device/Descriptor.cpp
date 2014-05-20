/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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
#include "Driver/LX/Internal.hpp"
#include "Device/Internal.hpp"
#include "Device/Register.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Port/ConfiguredPort.hpp"
#include "Port/DumpPort.hpp"
#include "NMEA/Info.hpp"
#include "Thread/Mutex.hpp"
#include "Util/StringUtil.hpp"
#include "Logger/NMEALogger.hpp"
#include "Language/Language.hpp"
#include "Operation/Operation.hpp"
#include "OS/Clock.hpp"
#include "../Simulator.hpp"
#include "Input/InputQueue.hpp"
#include "LogFile.hpp"
#include "Job/Job.hpp"

#ifdef ANDROID
#include "Java/Object.hpp"
#include "Java/Global.hpp"
#include "Android/InternalSensors.hpp"
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Android/Product.hpp"
#include "Android/IOIOHelper.hpp"
#include "Android/BMP085Device.hpp"
#include "Android/I2CbaroDevice.hpp"
#include "Android/BaroDevice.hpp"
#include "Android/NunchuckDevice.hpp"
#include "Android/VoltageDevice.hpp"
#endif

#ifdef __APPLE__
#include "Apple/InternalSensors.hpp"
#endif

#include <assert.h>

/**
 * This scope class calls DeviceDescriptor::Return() and
 * DeviceDescriptor::EnableNMEA() when the caller leaves the current
 * scope.  The caller must have called DeviceDescriptor::Borrow()
 * successfully before constructing this class.
 */
struct ScopeReturnDevice {
  DeviceDescriptor &device;
  OperationEnvironment &env;

  ScopeReturnDevice(DeviceDescriptor &_device, OperationEnvironment &_env)
    :device(_device), env(_env) {
  }

  ~ScopeReturnDevice() {
    device.EnableNMEA(env);
    device.Return();
  }
};

class OpenDeviceJob final : public Job {
  DeviceDescriptor &device;

public:
  OpenDeviceJob(DeviceDescriptor &_device):device(_device) {}

  /* virtual methods from class Job */
  virtual void Run(OperationEnvironment &env) {
    device.DoOpen(env);
  };
};

DeviceDescriptor::DeviceDescriptor(unsigned _index)
  :index(_index),
   open_job(NULL),
   port(NULL), monitor(NULL), dispatcher(NULL),
   driver(NULL), device(NULL),
#if defined(ANDROID) || defined(__APPLE__)
   internal_sensors(NULL),
#endif
#ifdef ANDROID
   droidsoar_v2(nullptr),
   nunchuck(nullptr),
   voltage(nullptr),
#endif
   n_failures(0u),
   ticker(false), borrowed(false)
{
  config.Clear();

#ifdef ANDROID
  for (unsigned i=0; i<sizeof i2cbaro/sizeof i2cbaro[0]; i++)
    i2cbaro[i] = nullptr;
  for (unsigned i=0; i<sizeof baro/sizeof baro[0]; i++)
    baro[i] = nullptr;
#endif
}

void
DeviceDescriptor::SetConfig(const DeviceConfig &_config)
{
  ResetFailureCounter();

  config = _config;

  if (config.UsesDriver()) {
    driver = FindDriverByName(config.driver_name);
    assert(driver != NULL);
  } else
    driver = NULL;
}

void
DeviceDescriptor::ClearConfig()
{
  config.Clear();
}

PortState
DeviceDescriptor::GetState() const
{
  if (open_job != nullptr)
    return PortState::LIMBO;

  if (port != nullptr)
    return port->GetState();

#if defined(ANDROID) || defined(__APPLE__)
  if (internal_sensors != nullptr)
    return PortState::READY;
#endif

#ifdef ANDROID
  if (droidsoar_v2 != nullptr)
    return PortState::READY;

  if (i2cbaro[0] != nullptr)
    return PortState::READY;

  if (baro[0] != nullptr)
    return PortState::READY;

  if (nunchuck != nullptr)
    return PortState::READY;

  if (voltage != nullptr)
    return PortState::READY;
#endif

  return PortState::FAILED;
}

bool
DeviceDescriptor::IsDumpEnabled() const
{
  return port != nullptr && port->IsEnabled();
}

void
DeviceDescriptor::DisableDump()
{
  if (port != nullptr)
    port->Disable();
}

void
DeviceDescriptor::EnableDumpTemporarily(unsigned duration_ms)
{
  if (port != nullptr)
    port->EnableTemporarily(duration_ms);
}

bool
DeviceDescriptor::IsCalibrating() const
{
  return (baro[0] != nullptr && baro[0]->IsCalibrating());
}

bool
DeviceDescriptor::IsCalibrable() const
{
  return (baro[0] != nullptr && !baro[0]->IsCalibrating());
}

void
DeviceDescriptor::Calibrate(fixed value)
{
  if (baro[0] != nullptr)
    baro[0]->Calibrate(value);
}

bool
DeviceDescriptor::ShouldReopenDriverOnTimeout() const
{
  return driver == NULL || driver->HasTimeout();
}

void
DeviceDescriptor::CancelAsync()
{
  assert(InMainThread());

  if (!async.IsBusy())
    return;

  assert(open_job != NULL);

  async.Cancel();
  async.Wait();

  delete open_job;
  open_job = NULL;
}

bool
DeviceDescriptor::OpenOnPort(DumpPort *_port, OperationEnvironment &env)
{
  assert(_port != NULL);
  assert(port == NULL);
  assert(device == NULL);
  assert(driver != NULL);
  assert(!ticker);
  assert(!IsBorrowed());

  reopen_clock.Update();

  device_blackboard->mutex.Lock();
  device_blackboard->SetRealState(index).Reset();
  device_blackboard->ScheduleMerge();
  device_blackboard->mutex.Unlock();

  settings_sent.Clear();
  settings_received.Clear();
  was_alive = false;

  port = _port;

  parser.Reset();
  parser.SetReal(_tcscmp(driver->name, _T("Condor")) != 0);
  parser.SetIgnoreChecksum(config.ignore_checksum);
  if (config.IsDriver(_T("Condor")))
    parser.DisableGeoid();

  if (driver->CreateOnPort != nullptr) {
    Device *new_device = driver->CreateOnPort(config, *port);

    const ScopeLock protect(mutex);
    device = new_device;
  } else
    port->StartRxThread();

  EnableNMEA(env);

  if (env.IsCancelled()) {
    /* the caller is responsible for freeing the port on error */
    port = nullptr;
    delete device;
    device = nullptr;
    return false;
  }

  return true;
}

bool
DeviceDescriptor::OpenInternalSensors()
{
#if defined(ANDROID) || defined(__APPLE__)
  if (is_simulator())
    return true;

#ifdef ANDROID
  internal_sensors =
      InternalSensors::create(Java::GetEnv(), context, GetIndex());
  if (internal_sensors) {
    // TODO: Allow user to specify whether they want certain sensors.
    internal_sensors->subscribeToSensor(InternalSensors::TYPE_MAGNETIC_FIELD);
    return true;
  }
#elif defined(__APPLE__)
  internal_sensors = InternalSensors::create(GetIndex());
  return (internal_sensors != nullptr);
#endif
#endif
  return false;
}

bool
DeviceDescriptor::OpenDroidSoarV2()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  if (i2cbaro[0] == NULL) {
    i2cbaro[0] = new I2CbaroDevice(GetIndex(), Java::GetEnv(), 
                       ioio_helper->GetHolder(),
                       DeviceConfig::PressureUse::STATIC_WITH_VARIO,
                       config.sensor_offset,
                       2 + (0x77 << 8) + (27 << 16), 0,	// bus, address
                       5,                               // update freq.
                       0);                              // flags

    i2cbaro[1] = new I2CbaroDevice(GetIndex(), Java::GetEnv(),
                       ioio_helper->GetHolder(),
                       // needs calibration ?
                       (config.sensor_factor == fixed(0)) ? DeviceConfig::PressureUse::PITOT_ZERO :
                                                            DeviceConfig::PressureUse::PITOT,
                       config.sensor_offset, 1 + (0x77 << 8) + (46 << 16), 0 ,
                       5,
                       0);
    return true;
  }
#endif
  return false;
}

bool
DeviceDescriptor::OpenI2Cbaro()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  for (unsigned i=0; i<sizeof i2cbaro/sizeof i2cbaro[0]; i++) {
    if (i2cbaro[i] == NULL) {
      i2cbaro[i] = new I2CbaroDevice(GetIndex(), Java::GetEnv(),
                       ioio_helper->GetHolder(),
                       // needs calibration ?
                       (config.sensor_factor == fixed(0) && config.press_use == DeviceConfig::PressureUse::PITOT) ?
                                          DeviceConfig::PressureUse::PITOT_ZERO :
                                          config.press_use,
                       config.sensor_offset,
                       config.i2c_bus, config.i2c_addr,
                       config.press_use == DeviceConfig::PressureUse::TEK_PRESSURE ? 20 : 5,
                       0); // called flags, actually reserved for future use.
      return true;
    }
  }
#endif
  return false;
}

bool
DeviceDescriptor::OpenBaro()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  for (unsigned i=0; i<sizeof baro/sizeof baro[0]; i++) {
    if (baro[i] == NULL) {
      baro[i] = new BaroDevice(GetIndex(), Java::GetEnv(),
                       ioio_helper->GetHolder(),
                       // needs calibration ?
                       (config.sensor_factor == fixed(0) && config.press_use == DeviceConfig::PressureUse::PITOT) ?
                                          DeviceConfig::PressureUse::PITOT_ZERO :
                                          config.press_use,
                       config.sensor_offset, config.sensor_factor,
                       config.press_type, config.i2c_bus, config.i2c_addr,
                       config.press_use == DeviceConfig::PressureUse::TEK_PRESSURE ? 20 : 5,
                       0); // called flags, actually reserved for future use.
      return true;
    }
  }
#endif
  return false;
}

bool
DeviceDescriptor::OpenNunchuck()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  nunchuck = new NunchuckDevice(GetIndex(), Java::GetEnv(),
                                  ioio_helper->GetHolder(),
                                  config.i2c_bus, 5); // twi, sample_rate
  return true;
#else
  return false;
#endif
}

bool
DeviceDescriptor::OpenVoltage()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  voltage = new VoltageDevice(GetIndex(), Java::GetEnv(),
                                  ioio_helper->GetHolder(),
                                  config.sensor_offset, config.sensor_factor,
                                  60); // sample_rate per minute
  return true;
#else
  return false;
#endif
}

bool
DeviceDescriptor::DoOpen(OperationEnvironment &env)
{
  assert(config.IsAvailable());

  if (config.port_type == DeviceConfig::PortType::INTERNAL)
    return OpenInternalSensors();

  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2)
    return OpenDroidSoarV2();

  if (config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR)
    return OpenI2Cbaro();

  if (config.port_type == DeviceConfig::PortType::IOIOPRESSURE)
    return OpenBaro();

  if (config.port_type == DeviceConfig::PortType::NUNCHUCK)
    return OpenNunchuck();

  if (config.port_type == DeviceConfig::PortType::IOIOVOLTAGE)
    return OpenVoltage();

  reopen_clock.Update();

  Port *port = OpenPort(config, *this);
  if (port == NULL) {
    TCHAR name_buffer[64];
    const TCHAR *name = config.GetPortName(name_buffer, 64);

    StaticString<256> msg;
    msg.Format(_T("%s: %s."), _("Unable to open port"), name);
    env.SetErrorMessage(msg);
    return false;
  }

  DumpPort *dump_port = new DumpPort(port);
  dump_port->Disable();

  if (!port->WaitConnected(env) || !OpenOnPort(dump_port, env)) {
    if (!env.IsCancelled())
      ++n_failures;

    delete dump_port;
    return false;
  }

  ResetFailureCounter();
  return true;
}

void
DeviceDescriptor::Open(OperationEnvironment &env)
{
  assert(InMainThread());
  assert(port == NULL);
  assert(device == NULL);
  assert(!ticker);
  assert(!IsBorrowed());

  if (is_simulator() || !config.IsAvailable())
    return;

  CancelAsync();

  assert(!IsOccupied());
  assert(open_job == NULL);

  TCHAR buffer[64];
  LogFormat(_T("Opening device %s"), config.GetPortName(buffer, 64));

  open_job = new OpenDeviceJob(*this);
  async.Start(open_job, env, this);
}

void
DeviceDescriptor::Close()
{
  assert(InMainThread());
  assert(!IsBorrowed());

  CancelAsync();

#if defined(ANDROID) || defined(__APPLE__)
  delete internal_sensors;
  internal_sensors = nullptr;
#endif

#ifdef ANDROID
  delete droidsoar_v2;
  droidsoar_v2 = nullptr;

  for (unsigned i=0; i<sizeof i2cbaro/sizeof i2cbaro[0]; i++) {
    delete i2cbaro[i];
    i2cbaro[i] = nullptr;
  }
  for (unsigned i=0; i<sizeof baro/sizeof baro[0]; i++) {
    delete baro[i];
    baro[i] = nullptr;
  }
  delete nunchuck;
  nunchuck = nullptr;

  delete voltage;
  voltage = nullptr;

#endif

  /* safely delete the Device object */
  Device *old_device = device;

  {
    const ScopeLock protect(mutex);
    device = nullptr;
    /* after leaving this scope, no other thread may use the old
       object; to avoid locking the mutex for too long, the "delete"
       is called after the scope */
  }

  delete old_device;

  Port *old_port = port;
  port = NULL;
  delete old_port;

  ticker = false;

  device_blackboard->mutex.Lock();
  device_blackboard->SetRealState(index).Reset();
  device_blackboard->ScheduleMerge();
  device_blackboard->mutex.Unlock();

  settings_sent.Clear();
  settings_received.Clear();
}

void
DeviceDescriptor::Reopen(OperationEnvironment &env)
{
  assert(InMainThread());
  assert(!IsBorrowed());

  Close();
  Open(env);
}

void
DeviceDescriptor::AutoReopen(OperationEnvironment &env)
{
  assert(InMainThread());

  if (/* don't reopen a device that is occupied */
      IsOccupied() ||
      !config.IsAvailable() ||
      !ShouldReopen() ||
      /* attempt to reopen a failed device every 30 seconds */
      !reopen_clock.CheckUpdate(30000))
    return;

#ifdef ANDROID
  if (config.port_type == DeviceConfig::PortType::RFCOMM &&
      native_view->GetAPILevel() < 11 && n_failures >= 2) {
    /* on Android < 3.0, system_server's "BT EventLoop" thread
       eventually crashes with JNI reference table overflow due to a
       memory leak after too many Bluetooth failures
       (https://code.google.com/p/android/issues/detail?id=8676);
       don't attempt to reconnect on this Android version over and
       over to keep the chance of this bug occurring low enough */

    if (n_failures == 2) {
      LogFormat(_T("Giving up on Bluetooth device %s to avoid Android crash bug"),
                config.bluetooth_mac.c_str());
      ++n_failures;
    }

    return;
  }
#endif

  TCHAR buffer[64];
  LogFormat(_T("Reconnecting to device %s"), config.GetPortName(buffer, 64));

  InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
  Reopen(env);
}

bool
DeviceDescriptor::EnableNMEA(OperationEnvironment &env)
{
  if (device == NULL)
    return true;

  bool success = device->EnableNMEA(env);

  if (port != NULL)
    /* re-enable the NMEA handler if it has been disabled by the
       driver */
    port->StartRxThread();

  return success;
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
  if (driver != NULL) {
    if (driver->IsManageable())
      return true;

    if (_tcscmp(driver->name, _T("LX")) == 0 && device != NULL) {
      const LXDevice &lx = *(const LXDevice *)device;
      return lx.IsV7() || lx.IsNano() || lx.IsLX16xx();
    }
  }

  return false;
}

bool
DeviceDescriptor::Borrow()
{
  assert(InMainThread());

  if (!CanBorrow())
    return false;

  borrowed = true;
  return true;
}

void
DeviceDescriptor::Return()
{
  assert(InMainThread());
  assert(IsBorrowed());

  borrowed = false;
  assert(!IsOccupied());

  /* if the caller has disabled the NMEA while the device was
     borrowed, we may not have received new values for some time, but
     that doesn't mean the device has failed; give it some time to
     recover, and don't reopen right away */
  reopen_clock.Update();
}

bool
DeviceDescriptor::IsAlive() const
{
  ScopeLock protect(device_blackboard->mutex);
  return device_blackboard->RealState(index).alive;
}

bool
DeviceDescriptor::ParseNMEA(const char *line, NMEAInfo &info)
{
  assert(line != NULL);

  /* restore the driver's ExternalSettings */
  const ExternalSettings old_settings = info.settings;
  info.settings = settings_received;

  if (device != NULL && device->ParseNMEA(line, info)) {
    info.alive.Update(info.clock);

    if (!config.sync_from_device)
      info.settings = old_settings;

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
    info.alive.Update(fixed(MonotonicClockMS()) / 1000);
    return true;
  }

  return false;
}

void
DeviceDescriptor::ForwardLine(const char *line)
{
  /* XXX make this method thread-safe; this method can be called from
     any thread, and if the Port gets closed, bad things happen */

  if (IsNMEAOut() && port != NULL) {
    Port *p = port;
    p->Write(line);
    p->Write("\r\n");
  }
}

bool
DeviceDescriptor::WriteNMEA(const char *line, OperationEnvironment &env)
{
  assert(line != NULL);

  return port != NULL && PortWriteNMEA(*port, line, env);
}

#ifdef _UNICODE
bool
DeviceDescriptor::WriteNMEA(const TCHAR *line, OperationEnvironment &env)
{
  assert(line != NULL);

  if (port == NULL)
    return false;

  char buffer[_tcslen(line) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, line, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return false;

  return WriteNMEA(buffer, env);
}
#endif

bool
DeviceDescriptor::PutMacCready(fixed value, OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || settings_sent.CompareMacCready(value) ||
      !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  if (!device->PutMacCready(value, env))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.mac_cready = value;
  settings_sent.mac_cready_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutBugs(fixed value, OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || settings_sent.CompareBugs(value) ||
      !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  if (!device->PutBugs(value, env))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.bugs = value;
  settings_sent.bugs_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutBallast(fixed fraction, fixed overload,
                             OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || !config.sync_to_device ||
      (settings_sent.CompareBallastFraction(fraction) &&
       settings_sent.CompareBallastOverload(overload)))
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  if (!device->PutBallast(fraction, overload, env))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.ballast_fraction = fraction;
  settings_sent.ballast_fraction_available.Update(basic.clock);
  settings_sent.ballast_overload = overload;
  settings_sent.ballast_overload_available.Update(basic.clock);

  return true;
}

bool
DeviceDescriptor::PutVolume(unsigned volume, OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  return device->PutVolume(volume, env);
}

bool
DeviceDescriptor::PutActiveFrequency(RadioFrequency frequency,
                                     const TCHAR *name,
                                     OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  return device->PutActiveFrequency(frequency, name, env);
}

bool
DeviceDescriptor::PutStandbyFrequency(RadioFrequency frequency,
                                      const TCHAR *name,
                                      OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  return device->PutStandbyFrequency(frequency, name, env);
}

bool
DeviceDescriptor::PutQNH(const AtmosphericPressure &value,
                         OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == NULL || settings_sent.CompareQNH(value) ||
      !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);
  if (!device->PutQNH(value, env))
    return false;

  ScopeLock protect(device_blackboard->mutex);
  NMEAInfo &basic = device_blackboard->SetRealState(index);
  settings_sent.qnh = value;
  settings_sent.qnh_available.Update(basic.clock);

  return true;
}

static bool
DeclareToFLARM(const struct Declaration &declaration, Port &port,
               const Waypoint *home, OperationEnvironment &env)
{
  return FlarmDevice(port).Declare(declaration, home, env);
}

static bool
DeclareToFLARM(const struct Declaration &declaration,
               Port &port, const DeviceRegister &driver, Device *device,
               const Waypoint *home,
               OperationEnvironment &env)
{
  /* enable pass-through mode in the "front" device */
  if (driver.HasPassThrough() && device != NULL &&
      !device->EnablePassThrough(env))
    return false;

  return DeclareToFLARM(declaration, port, home, env);
}

static bool
DoDeclare(const struct Declaration &declaration,
          Port &port, const DeviceRegister &driver, Device *device,
          bool flarm, const Waypoint *home,
          OperationEnvironment &env)
{
  StaticString<60> text;
  text.Format(_T("%s: %s."), _("Sending declaration"), driver.display_name);
  env.SetText(text);

  bool result = device != NULL && device->Declare(declaration, home, env);

  if (flarm) {
    text.Format(_T("%s: FLARM."), _("Sending declaration"));
    env.SetText(text);

    result |= DeclareToFLARM(declaration, port, driver, device, home, env);
  }

  return result;
}

bool
DeviceDescriptor::Declare(const struct Declaration &declaration,
                          const Waypoint *home,
                          OperationEnvironment &env)
{
  assert(borrowed);
  assert(port != NULL);
  assert(driver != NULL);
  assert(device != NULL);

  /* enable the "muxed FLARM" hack? */
  const bool flarm = device_blackboard->IsFLARM(index) &&
    !IsDriver(_T("FLARM"));

  return DoDeclare(declaration, *port, *driver, device, flarm,
                   home, env);
}

bool
DeviceDescriptor::ReadFlightList(RecordedFlightList &flight_list,
                                 OperationEnvironment &env)
{
  assert(borrowed);
  assert(port != NULL);
  assert(driver != NULL);
  assert(device != NULL);

  StaticString<60> text;
  text.Format(_T("%s: %s."), _("Reading flight list"), driver->display_name);
  env.SetText(text);

  return device->ReadFlightList(flight_list, env);
}

bool
DeviceDescriptor::DownloadFlight(const RecordedFlightInfo &flight,
                                 const TCHAR *path,
                                 OperationEnvironment &env)
{
  assert(borrowed);
  assert(port != NULL);
  assert(driver != NULL);
  assert(device != NULL);

  if (port == NULL || driver == NULL || device == NULL)
    return false;

  StaticString<60> text;
  text.Format(_T("%s: %s."), _("Downloading flight log"), driver->display_name);
  env.SetText(text);

  return device->DownloadFlight(flight, path, env);
}

void
DeviceDescriptor::OnSysTicker()
{
  assert(InMainThread());

  if (port != NULL && port->GetState() == PortState::FAILED && !IsOccupied())
    Close();

  if (device == NULL)
    return;

  const bool now_alive = IsAlive();
  if (!now_alive && was_alive && !IsOccupied()) {
    /* connection was just lost */
    device->LinkTimeout();

    NullOperationEnvironment env;
    EnableNMEA(env);
  }

  was_alive = now_alive;

  if (now_alive || IsBorrowed()) {
    ticker = !ticker;
    if (ticker)
      // write settings to vario every second
      device->OnSysTicker();
  }
}

void
DeviceDescriptor::OnSensorUpdate(const MoreData &basic)
{
  /* must hold the mutex because this method may run in any thread,
     just in case the main thread deletes the Device while this method
     still runs */
  const ScopeLock protect(mutex);

  if (device != nullptr)
    device->OnSensorUpdate(basic);
}

void
DeviceDescriptor::OnCalculatedUpdate(const MoreData &basic,
                                     const DerivedInfo &calculated)
{
  assert(InMainThread());

  if (device != nullptr)
    device->OnCalculatedUpdate(basic, calculated);
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
DeviceDescriptor::OnNotification()
{
  /* notification from AsyncJobRunner, the Job was finished */

  assert(InMainThread());
  assert(open_job != NULL);

  async.Wait();

  delete open_job;
  open_job = NULL;
}

void
DeviceDescriptor::DataReceived(const void *data, size_t length)
{
  if (monitor != NULL)
    monitor->DataReceived(data, length);

  // Pass data directly to drivers that use binary data protocols
  if (driver != NULL && device != NULL && driver->UsesRawData()) {
    ScopeLock protect(device_blackboard->mutex);
    NMEAInfo &basic = device_blackboard->SetRealState(index);
    basic.UpdateClock();

    const ExternalSettings old_settings = basic.settings;

    if (device->DataReceived(data, length, basic)) {
      if (!config.sync_from_device)
        basic.settings = old_settings;

      device_blackboard->ScheduleMerge();
    }

    return;
  }

  if (!IsNMEAOut())
    PortLineSplitter::DataReceived(data, length);
}

void
DeviceDescriptor::LineReceived(const char *line)
{
  NMEALogger::Log(line);

  if (dispatcher != NULL)
    dispatcher->LineReceived(line);

  if (ParseLine(line))
    device_blackboard->ScheduleMerge();
}
