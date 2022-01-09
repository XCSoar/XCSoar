/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Descriptor.hpp"
#include "DataEditor.hpp"
#include "Driver.hpp"
#include "Parser.hpp"
#include "Util/NMEAWriter.hpp"
#include "Register.hpp"
#include "Driver/FLARM/Device.hpp"
#include "Driver/LX/Internal.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Components.hpp"
#include "Port/ConfiguredPort.hpp"
#include "Port/DumpPort.hpp"
#include "NMEA/Info.hpp"
#include "thread/Mutex.hxx"
#include "util/StringAPI.hxx"
#include "util/ConvertString.hpp"
#include "util/Exception.hxx"
#include "Logger/NMEALogger.hpp"
#include "Language/Language.hpp"
#include "Operation/Operation.hpp"
#include "Operation/Cancelled.hpp"
#include "system/Path.hpp"
#include "../Simulator.hpp"
#include "Input/InputQueue.hpp"
#include "LogFile.hpp"
#include "Job/Job.hpp"

#ifdef ANDROID
#include "java/Object.hxx"
#include "java/Closeable.hxx"
#include "java/Global.hxx"
#include "Android/BluetoothHelper.hpp"
#include "Android/InternalSensors.hpp"
#include "Android/GliderLink.hpp"
#include "Android/Main.hpp"
#include "Android/Product.hpp"
#include "Android/IOIOHelper.hpp"
#include "Android/I2CbaroDevice.hpp"
#include "Android/NunchuckDevice.hpp"
#include "Android/VoltageDevice.hpp"
#include "Android/Sensor.hpp"
#endif

#ifdef __APPLE__
#include "Apple/InternalSensors.hpp"
#endif

#include <cassert>

class OpenDeviceJob final : public Job {
  DeviceDescriptor &device;

public:
  explicit OpenDeviceJob(DeviceDescriptor &_device) noexcept
    :device(_device) {}

  /* virtual methods from class Job */
  void Run(OperationEnvironment &env) override {
    device.DoOpen(env);
  };
};

DeviceDescriptor::DeviceDescriptor(EventLoop &_event_loop,
                                   Cares::Channel &_cares,
                                   unsigned _index,
                                   PortListener *_port_listener) noexcept
  :event_loop(_event_loop), cares(_cares), index(_index),
   port_listener(_port_listener)
{
  config.Clear();
}

DeviceDescriptor::~DeviceDescriptor() noexcept
{
  assert(!IsOccupied());
}

void
DeviceDescriptor::SetConfig(const DeviceConfig &_config) noexcept
{
  ResetFailureCounter();

  config = _config;

  if (config.UsesDriver()) {
    driver = FindDriverByName(config.driver_name);
    assert(driver != nullptr);
    second_driver = nullptr;
    if (driver->HasPassThrough() && config.use_second_device){
      second_driver = FindDriverByName(config.driver2_name);
      assert(second_driver != nullptr);
    }
  } else{
    driver = nullptr;
    second_driver = nullptr;
  }
}

void
DeviceDescriptor::ClearConfig() noexcept
{
  config.Clear();
}

PortState
DeviceDescriptor::GetState() const noexcept
{
  if (has_failed)
    return PortState::FAILED;

  if (open_job != nullptr)
    return PortState::LIMBO;

  if (port != nullptr)
    return port->GetState();

#ifdef ANDROID
  if (java_sensor != nullptr)
    return AndroidSensor::GetState(Java::GetEnv(), *java_sensor);
#endif

  return PortState::FAILED;
}

bool
DeviceDescriptor::IsDumpEnabled() const noexcept
{
  return port != nullptr && port->IsEnabled();
}

void
DeviceDescriptor::DisableDump() noexcept
{
  if (port != nullptr)
    port->Disable();
}

void
DeviceDescriptor::EnableDumpTemporarily(std::chrono::steady_clock::duration duration) noexcept
{
  if (port != nullptr)
    port->EnableTemporarily(duration);
}

bool
DeviceDescriptor::ShouldReopenDriverOnTimeout() const noexcept
{
  return driver == nullptr || driver->HasTimeout();
}

void
DeviceDescriptor::CancelAsync() noexcept
{
  assert(InMainThread());

  if (!async.IsBusy())
    return;

  assert(open_job != nullptr);

  async.Cancel();

  try {
    async.Wait();
  } catch (OperationCancelled) {
  } catch (...) {
    LogError(std::current_exception());
  }

  delete open_job;
  open_job = nullptr;
}

inline bool
DeviceDescriptor::OpenOnPort(std::unique_ptr<DumpPort> &&_port, OperationEnvironment &env)
try {
  assert(port == nullptr);
  assert(device == nullptr);
  assert(second_device == nullptr);
  assert(driver != nullptr);
  assert(!ticker);
  assert(!IsBorrowed());

  reopen_clock.Update();

  {
    const auto e = BeginEdit();
    e->Reset();
    e.Commit();
  }

  settings_sent.Clear();
  settings_received.Clear();
  was_alive = false;

  port = std::move(_port);

  parser.Reset();
  parser.SetReal(!StringIsEqual(driver->name, _T("Condor")));
  if (config.IsDriver(_T("Condor")))
    parser.DisableGeoid();

  if (driver->CreateOnPort != nullptr) {
    Device *new_device = driver->CreateOnPort(config, *port);

    const std::lock_guard<Mutex> lock(mutex);
    device = new_device;

    if (driver->HasPassThrough() && config.use_second_device)
      second_device = second_driver->CreateOnPort(config, *port);
  } else
    port->StartRxThread();

  EnableNMEA(env);

  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  port = nullptr;
  delete device;
  device = nullptr;
  delete second_device;
  second_device = nullptr;
  throw;
}

bool
DeviceDescriptor::OpenInternalSensors()
{
#ifdef HAVE_INTERNAL_GPS
  if (is_simulator())
    return true;

#ifdef ANDROID
  internal_sensors =
      InternalSensors::create(Java::GetEnv(), context, *this);
  if (internal_sensors) {
    // TODO: Allow user to specify whether they want certain sensors.
    internal_sensors->subscribeToSensor(InternalSensors::TYPE_PRESSURE);
    internal_sensors->subscribeToSensor(InternalSensors::TYPE_ACCELEROMETER);
    return true;
  }
#elif defined(__APPLE__)
  internal_sensors = new InternalSensors(*this);
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

  /* we use different values for the I2C Kalman filter */
  kalman_filter = {KF_I2C_MAX_DT, KF_I2C_VAR_ACCEL};

  auto i2c = I2CbaroDevice::Create(Java::GetEnv(),
                                   ioio_helper->GetHolder(),
                                   0,
                                   2 + (0x77 << 8) + (27 << 16), 0,	// bus, address
                                   5,                               // update freq.
                                   0,                               // flags
                                   *this);
  java_sensor = new Java::GlobalCloseable(i2c);

  i2c = I2CbaroDevice::Create(Java::GetEnv(),
                              ioio_helper->GetHolder(),
                              1,
                              1 + (0x77 << 8) + (46 << 16), 0 ,
                              5,
                              0,
                              *this);
  second_java_sensor = new Java::GlobalCloseable(i2c);

  return true;
#else
  return false;
#endif
}

bool
DeviceDescriptor::OpenI2Cbaro()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  /* we use different values for the I2C Kalman filter */
  kalman_filter = {KF_I2C_MAX_DT, KF_I2C_VAR_ACCEL};

  auto i2c = I2CbaroDevice::Create(Java::GetEnv(),
                                   ioio_helper->GetHolder(),
                                   0,
                                   config.i2c_bus, config.i2c_addr,
                                   config.press_use == DeviceConfig::PressureUse::TEK_PRESSURE ? 20 : 5,
                                   0, // called flags, actually reserved for future use.
                                   *this);
  java_sensor = new Java::GlobalCloseable(i2c);

  return true;
#else
  return false;
#endif
}

bool
DeviceDescriptor::OpenNunchuck()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (ioio_helper == nullptr)
    return false;

  joy_state_x = joy_state_y = 0;

  auto nunchuk = NunchuckDevice::Create(Java::GetEnv(),
                                        ioio_helper->GetHolder(),
                                        config.i2c_bus, 5, // twi, sample_rate
                                        *this);
  java_sensor = new Java::GlobalCloseable(nunchuk);
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

  voltage_offset = config.sensor_offset;
  voltage_factor = config.sensor_factor;

  for (auto &i : voltage_filter)
    i.Reset();
  temperature_filter.Reset();

  auto voltage = VoltageDevice::Create(Java::GetEnv(),
                                       ioio_helper->GetHolder(),
                                       60, // sample_rate per minute
                                       *this);
  java_sensor = new Java::GlobalCloseable(voltage);
  return true;
#else
  return false;
#endif
}

bool
DeviceDescriptor::OpenGliderLink()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  java_sensor = new Java::GlobalCloseable(GliderLink::Create(Java::GetEnv(),
                                                             *context, *this));
  return true;
#else
  return false;
#endif
}

inline bool
DeviceDescriptor::OpenBluetoothSensor()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  if (bluetooth_helper == nullptr)
    throw std::runtime_error("Bluetooth not available");

  if (config.bluetooth_mac.empty())
    throw std::runtime_error("No Bluetooth MAC configured");

  java_sensor = new Java::GlobalCloseable(bluetooth_helper->connectSensor(Java::GetEnv(),
                                                                          config.bluetooth_mac,
                                                                          *this));
  return true;
#else
  return false;
#endif
}

bool
DeviceDescriptor::DoOpen(OperationEnvironment &env) noexcept
try {
  assert(config.IsAvailable());

  {
    std::lock_guard<Mutex> lock(mutex);
    error_message.clear();
  }

  if (config.port_type == DeviceConfig::PortType::INTERNAL)
    return OpenInternalSensors();

  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2)
    return OpenDroidSoarV2();

  if (config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR)
    return OpenI2Cbaro();

  if (config.port_type == DeviceConfig::PortType::NUNCHUCK)
    return OpenNunchuck();

  if (config.port_type == DeviceConfig::PortType::IOIOVOLTAGE)
    return OpenVoltage();

  if (config.port_type == DeviceConfig::PortType::GLIDER_LINK)
    return OpenGliderLink();

  if (config.port_type == DeviceConfig::PortType::BLE_SENSOR)
    return OpenBluetoothSensor();

  reopen_clock.Update();

  std::unique_ptr<Port> port;
  try {
    port = OpenPort(event_loop, cares, config, this, *this);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    const auto e = std::current_exception();

    TCHAR name_buffer[64];
    const TCHAR *name = config.GetPortName(name_buffer, 64);

    LogError(e, WideToUTF8Converter(name));

    StaticString<256> msg;

    const auto _msg = GetFullMessage(e);
    const UTF8ToWideConverter what(_msg.c_str());
    if (what.IsValid()) {
      std::lock_guard<Mutex> lock(mutex);
      error_message = what;
    }

    msg.Format(_T("%s: %s (%s)"), _("Unable to open port"), name,
               (const TCHAR *)what);

    env.SetErrorMessage(msg);
    return false;
  }

  if (port == nullptr) {
    TCHAR name_buffer[64];
    const TCHAR *name = config.GetPortName(name_buffer, 64);

    StaticString<256> msg;
    msg.Format(_T("%s: %s."), _("Unable to open port"), name);
    env.SetErrorMessage(msg);
    return false;
  }

  if (!port->WaitConnected(env)) {
    ++n_failures;
    return false;
  }

  auto dump_port = std::make_unique<DumpPort>(std::move(port));
  dump_port->Disable();

  if (!OpenOnPort(std::move(dump_port), env)) {
    ++n_failures;
    return false;
  }

  ResetFailureCounter();
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  const auto _msg = GetFullMessage(std::current_exception());
  const UTF8ToWideConverter msg(_msg.c_str());
  env.SetErrorMessage(msg);
  return false;
}

void
DeviceDescriptor::Open(OperationEnvironment &env)
{
  assert(InMainThread());
  assert(port == nullptr);
  assert(device == nullptr);
  assert(second_device == nullptr);
  assert(!has_failed);
  assert(!ticker);
  assert(!IsBorrowed());

  if (is_simulator() || !config.IsAvailable())
    return;

  CancelAsync();

  assert(!IsOccupied());
  assert(open_job == nullptr);

  TCHAR buffer[64];
  LogFormat(_T("Opening device %s"), config.GetPortName(buffer, 64));

#ifdef ANDROID
  /* reset the Kalman filter */
  kalman_filter = {KF_MAX_DT, KF_VAR_ACCEL};
#endif

  open_job = new OpenDeviceJob(*this);
  async.Start(open_job, env, &job_finished_notify);

  PortStateChanged();
}

void
DeviceDescriptor::Close() noexcept
{
  assert(InMainThread());
  assert(!IsBorrowed());

  CancelAsync();

#ifdef HAVE_INTERNAL_GPS
  delete internal_sensors;
  internal_sensors = nullptr;
#endif

#ifdef ANDROID
  delete second_java_sensor;
  second_java_sensor = nullptr;

  delete java_sensor;
  java_sensor = nullptr;
#endif

  /* safely delete the Device object */
  Device *old_device = device;

  {
    const std::lock_guard<Mutex> lock(mutex);
    device = nullptr;
    /* after leaving this scope, no other thread may use the old
       object; to avoid locking the mutex for too long, the "delete"
       is called after the scope */
  }

  delete old_device;

  delete second_device;
  second_device = nullptr;

  port.reset();

  has_failed = false;
  ticker = false;

  {
    const auto e = BeginEdit();
    e->Reset();
    e.Commit();
  }

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
      !reopen_clock.CheckUpdate(std::chrono::seconds(30)))
    return;

  TCHAR buffer[64];
  LogFormat(_T("Reconnecting to device %s"), config.GetPortName(buffer, 64));

  InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
  Reopen(env);
}

bool
DeviceDescriptor::EnableNMEA(OperationEnvironment &env) noexcept
{
  if (device == nullptr)
    return true;

  bool success = false;

  try {
    success = device->EnableNMEA(env);
  } catch (OperationCancelled) {
  } catch (...) {
    LogError(std::current_exception(), "EnableNMEA() failed");
  }

  if (port != nullptr)
    /* re-enable the NMEA handler if it has been disabled by the
       driver */
    port->StartRxThread();

  return success;
}

const TCHAR *
DeviceDescriptor::GetDisplayName() const noexcept
{
  return driver != nullptr
    ? driver->display_name
    : nullptr;
}

bool
DeviceDescriptor::IsDriver(const TCHAR *name) const noexcept
{
  return driver != nullptr
    ? StringIsEqual(driver->name, name)
    : false;
}

bool
DeviceDescriptor::CanDeclare() const noexcept
{
  return driver != nullptr &&
    (driver->CanDeclare() ||
     device_blackboard->IsFLARM(index));
}

bool
DeviceDescriptor::IsLogger() const noexcept
{
  return driver != nullptr && driver->IsLogger();
}

bool
DeviceDescriptor::IsNMEAOut() const noexcept
{
  return driver != nullptr && driver->IsNMEAOut();
}

bool
DeviceDescriptor::IsManageable() const noexcept
{
  if (driver != nullptr) {
    if (driver->IsManageable())
      return true;

    if (StringIsEqual(driver->name, _T("LX")) && device != nullptr) {
      const LXDevice &lx = *(const LXDevice *)device;
      return lx.IsManageable();
    }
  }

#ifdef ANDROID
  if (config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR)
      return config.press_use == DeviceConfig::PressureUse::PITOT;

  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2)
    return true;
#endif

  return false;
}

bool
DeviceDescriptor::Borrow() noexcept
{
  assert(InMainThread());

  if (!CanBorrow())
    return false;

  borrowed = true;
  return true;
}

void
DeviceDescriptor::Return() noexcept
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
DeviceDescriptor::IsAlive() const noexcept
{
  std::lock_guard<Mutex> lock(device_blackboard->mutex);
  return device_blackboard->RealState(index).alive;
}

TimeStamp
DeviceDescriptor::GetClock() const noexcept
{
  const std::lock_guard<Mutex> lock(device_blackboard->mutex);
  const NMEAInfo &basic = device_blackboard->RealState(index);
  return basic.clock;
}

NMEAInfo
DeviceDescriptor::GetData() const noexcept
{
  const std::lock_guard<Mutex> lock(device_blackboard->mutex);
  return device_blackboard->RealState(index);
}

DeviceDataEditor
DeviceDescriptor::BeginEdit() noexcept
{
  return {*device_blackboard, index};
}

bool
DeviceDescriptor::ParseNMEA(const char *line, NMEAInfo &info) noexcept
{
  assert(line != nullptr);

  /* restore the driver's ExternalSettings */
  const ExternalSettings old_settings = info.settings;
  info.settings = settings_received;

  if (device != nullptr && device->ParseNMEA(line, info)) {
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
    info.alive.Update(info.clock);
    return true;
  }

  return false;
}

void
DeviceDescriptor::ForwardLine(const char *line)
{
  /* XXX make this method thread-safe; this method can be called from
     any thread, and if the Port gets closed, bad things happen */

  if (IsNMEAOut() && port != nullptr) {
    Port *p = port.get();
    p->Write(line);
    p->Write("\r\n");
  }
}

bool
DeviceDescriptor::WriteNMEA(const char *line,
                            OperationEnvironment &env) noexcept
{
  assert(line != nullptr);

  if (port == nullptr)
      return false;

  try {
    PortWriteNMEA(*port, line, env);
    return true;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    env.SetError(std::current_exception());
    return false;
  }
}

#ifdef _UNICODE
bool
DeviceDescriptor::WriteNMEA(const TCHAR *line,
                            OperationEnvironment &env) noexcept
{
  assert(line != nullptr);

  if (port == nullptr)
    return false;

  WideToACPConverter narrow{line};
  return narrow.IsValid() && WriteNMEA(narrow, env);
}
#endif

bool
DeviceDescriptor::PutMacCready(double value,
                               OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || settings_sent.CompareMacCready(value) ||
      !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  ScopeReturnDevice restore(*this, env);

  try {
    if (!device->PutMacCready(value, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutMacCready() failed");
    return false;
  }

  settings_sent.mac_cready = value;
  settings_sent.mac_cready_available.Update(GetClock());

  return true;
}

bool
DeviceDescriptor::PutBugs(double value, OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || settings_sent.CompareBugs(value) ||
      !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    const ScopeReturnDevice restore(*this, env);
    if (!device->PutBugs(value, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutBugs() failed");
    return false;
  }

  settings_sent.bugs = value;
  settings_sent.bugs_available.Update(GetClock());

  return true;
}

bool
DeviceDescriptor::PutBallast(double fraction, double overload,
                             OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device ||
      (settings_sent.CompareBallastFraction(fraction) &&
       settings_sent.CompareBallastOverload(overload)))
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    const ScopeReturnDevice restore(*this, env);
    if (!device->PutBallast(fraction, overload, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutBallast() failed");
    return false;
  }

  const auto clock = GetClock();
  settings_sent.ballast_fraction = fraction;
  settings_sent.ballast_fraction_available.Update(clock);
  settings_sent.ballast_overload = overload;
  settings_sent.ballast_overload_available.Update(clock);

  return true;
}

bool
DeviceDescriptor::PutVolume(unsigned volume,
                            OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    return device->PutVolume(volume, env);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutVolume() failed");
    return false;
  }
}

bool
DeviceDescriptor::PutPilotEvent(OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    return device->PutPilotEvent(env);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutPilotEvent() failed");
    return false;
  }
}

bool
DeviceDescriptor::PutActiveFrequency(RadioFrequency frequency,
                                     const TCHAR *name,
                                     OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    return device->PutActiveFrequency(frequency, name, env);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutActiveFrequency() failed");
    return false;
  }
}

bool
DeviceDescriptor::PutStandbyFrequency(RadioFrequency frequency,
                                      const TCHAR *name,
                                      OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    return device->PutStandbyFrequency(frequency, name, env);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutStandbyFrequency() failed");
    return false;
  }
}

bool
DeviceDescriptor::PutQNH(const AtmosphericPressure value,
                         OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || settings_sent.CompareQNH(value) ||
      !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    if (!device->PutQNH(value, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutQNH() failed");
    return false;
  }

  settings_sent.qnh = value;
  settings_sent.qnh_available.Update(GetClock());

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
  if (driver.HasPassThrough() && device != nullptr &&
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

  bool result = device != nullptr && device->Declare(declaration, home, env);

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
  assert(port != nullptr);
  assert(driver != nullptr);
  assert(device != nullptr);

  // explicitly set passthrough device? Use it...
  if (driver->HasPassThrough() && second_device != nullptr) {
    // set the primary device to passthrough
    device->EnablePassThrough(env);
    return second_device != nullptr &&
      second_device->Declare(declaration, home, env);
  } else {
    /* enable the "muxed FLARM" hack? */
    const bool flarm = device_blackboard->IsFLARM(index) &&
      !IsDriver(_T("FLARM"));

    return DoDeclare(declaration, *port, *driver, device, flarm,
                     home, env);
  }
}

bool
DeviceDescriptor::ReadFlightList(RecordedFlightList &flight_list,
                                 OperationEnvironment &env)
{
  assert(borrowed);
  assert(port != nullptr);
  assert(driver != nullptr);
  assert(device != nullptr);

  StaticString<60> text;

  if (driver->HasPassThrough() && second_device != nullptr) {
    text.Format(_T("%s: %s."), _("Reading flight list"),
                second_driver->display_name);
    env.SetText(text);

    device->EnablePassThrough(env);
    return second_device->ReadFlightList(flight_list, env);
  } else {
    text.Format(_T("%s: %s."), _("Reading flight list"), driver->display_name);
    env.SetText(text);

    return device->ReadFlightList(flight_list, env);
  }
}

bool
DeviceDescriptor::DownloadFlight(const RecordedFlightInfo &flight,
                                 Path path,
                                 OperationEnvironment &env)
{
  assert(borrowed);
  assert(port != nullptr);
  assert(driver != nullptr);
  assert(device != nullptr);

  if (port == nullptr || driver == nullptr || device == nullptr)
    return false;

  StaticString<60> text;


  if (driver->HasPassThrough() && (second_device != nullptr)) {
    text.Format(_T("%s: %s."), _("Downloading flight log"),
                second_driver->display_name);
    env.SetText(text);

    device->EnablePassThrough(env);
    return second_device->DownloadFlight(flight, path, env);
  } else {
    text.Format(_T("%s: %s."), _("Downloading flight log"),
                driver->display_name);
    env.SetText(text);

    return device->DownloadFlight(flight, path, env);
  }
}

void
DeviceDescriptor::OnSysTicker() noexcept
{
  assert(InMainThread());

  if (port != nullptr && port->GetState() == PortState::FAILED)
    has_failed = true;

#ifdef ANDROID
  if (java_sensor != nullptr &&
      AndroidSensor::GetState(Java::GetEnv(), *java_sensor) == PortState::FAILED)
    has_failed = true;
#endif

  if (has_failed && !IsOccupied())
    Close();

  if (device == nullptr)
    return;

  const bool now_alive = IsAlive();
  if (!now_alive && was_alive && !IsOccupied()) {
    /* connection was just lost */
    try {
      device->LinkTimeout();
    } catch (...) {
      LogError(std::current_exception(), "LinkTimeout() failed");
    }

    NullOperationEnvironment env;
    EnableNMEA(env);
  }

  was_alive = now_alive;

  if (now_alive || IsBorrowed()) {
    ticker = !ticker;
    if (ticker)
      try {
        // write settings to vario every second
        device->OnSysTicker();
      } catch (...) {
        LogError(std::current_exception(), "OnSysTicker() failed");
      }
  }
}

void
DeviceDescriptor::OnSensorUpdate(const MoreData &basic) noexcept
{
  /* must hold the mutex because this method may run in any thread,
     just in case the main thread deletes the Device while this method
     still runs */
  const std::lock_guard<Mutex> lock(mutex);

  if (device != nullptr)
    try {
      device->OnSensorUpdate(basic);
    } catch (...) {
      LogError(std::current_exception(), "OnSensorUpdate() failed");
    }
}

void
DeviceDescriptor::OnCalculatedUpdate(const MoreData &basic,
                                     const DerivedInfo &calculated) noexcept
{
  assert(InMainThread());

  if (device != nullptr)
    try {
      device->OnCalculatedUpdate(basic, calculated);
    } catch (...) {
      LogError(std::current_exception(), "OnCalculatedUpdate() failed");
    }
}

void
DeviceDescriptor::OnJobFinished() noexcept
{
  /* notification from AsyncJobRunner, the Job was finished */

  assert(InMainThread());
  assert(open_job != nullptr);

  try {
    async.Wait();
  } catch (OperationCancelled) {
  } catch (...) {
    LogError(std::current_exception());
  }

  delete open_job;
  open_job = nullptr;

  PortStateChanged();
}

void
DeviceDescriptor::PortStateChanged() noexcept
{
  if (port_listener != nullptr)
    port_listener->PortStateChanged();
}

void
DeviceDescriptor::PortError(const char *msg) noexcept
{
  {
    TCHAR buffer[64];
    LogFormat(_T("Error on device %s: %s"),
              config.GetPortName(buffer, 64), msg);
  }

  {
    const UTF8ToWideConverter tmsg(msg);
    if (tmsg.IsValid()) {
      std::lock_guard<Mutex> lock(mutex);
      error_message = tmsg;
    }
  }

  has_failed = true;

  if (port_listener != nullptr)
    port_listener->PortError(msg);
}

bool
DeviceDescriptor::DataReceived(std::span<const std::byte> s) noexcept
{
  if (monitor != nullptr)
    monitor->DataReceived(s);

  // Pass data directly to drivers that use binary data protocols
  if (driver != nullptr && device != nullptr && driver->UsesRawData()) {
    auto basic = device_blackboard->LockGetDeviceDataUpdateClock(index);

    const ExternalSettings old_settings = basic.settings;

    /* call Device::DataReceived() without holding
       DeviceBlackboard::mutex to avoid blocking all other threads */
    if (device->DataReceived(s, basic)) {
      if (!config.sync_from_device)
        basic.settings = old_settings;

      device_blackboard->LockSetDeviceDataScheuduleMerge(index, basic);
    }

    return true;
  }

  if (!IsNMEAOut())
    PortLineSplitter::DataReceived(s);

  return true;
}

bool
DeviceDescriptor::LineReceived(const char *line) noexcept
{
  if (nmea_logger != nullptr)
    nmea_logger->Log(line);

  if (dispatcher != nullptr)
    dispatcher->LineReceived(line);

  const auto e = BeginEdit();
  e->UpdateClock();
  ParseNMEA(line, *e);
  e.Commit();

  return true;
}
