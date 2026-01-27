// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Descriptor.hpp"
#include "Factory.hpp"
#include "DataEditor.hpp"
#include "Driver.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Geo/GeoPoint.hpp"
#include "Parser.hpp"
#include "Util/NMEAWriter.hpp"
#include "Register.hpp"
#include "Driver/FLARM/Device.hpp"
#include "Driver/LX/Internal.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Port/ConfiguredPort.hpp"
#include "Port/DumpPort.hpp"
#include "NMEA/Info.hpp"
#include "thread/Mutex.hxx"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
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
#include "java/Closeable.hxx"
#include "java/Global.hxx"
#include "Android/InternalSensors.hpp"
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

DeviceDescriptor::DeviceDescriptor(DeviceBlackboard &_blackboard,
                                   NMEALogger *_nmea_logger,
                                   DeviceFactory &_factory,
                                   unsigned _index,
                                   PortListener *_port_listener) noexcept
  :blackboard(_blackboard), nmea_logger(_nmea_logger),
   factory(_factory),
   index(_index),
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

  if (internal_sensors != nullptr)
    return internal_sensors->GetState(Java::GetEnv());
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
  parser.SetReal(!StringIsEqual(driver->name, "Condor"));
  if (config.IsDriver("Condor"))
    parser.DisableGeoid();

  if (driver->CreateOnPort != nullptr) {
    Device *new_device = driver->CreateOnPort(config, *port);

    const std::lock_guard lock{mutex};
    device = new_device;

    if (driver->HasPassThrough() && config.use_second_device &&
        second_driver->CreateOnPort != nullptr)
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

inline bool
DeviceDescriptor::OpenInternalSensors()
{
#ifdef HAVE_INTERNAL_GPS
  if (is_simulator())
    return true;

  internal_sensors = factory.OpenInternalSensors(*this);
  return internal_sensors != nullptr;
#else
  return false;
#endif
}

inline bool
DeviceDescriptor::OpenDroidSoarV2()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  /* we use different values for the I2C Kalman filter */
  kalman_filter = {KF_I2C_MAX_DT, KF_I2C_VAR_ACCEL};

  auto [a, b] = factory.OpenDroidSoarV2(*this);
  java_sensor = new Java::GlobalCloseable(a);
  second_java_sensor = new Java::GlobalCloseable(b);

  return true;
#else
  return false;
#endif
}

inline bool
DeviceDescriptor::OpenI2Cbaro()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  /* we use different values for the I2C Kalman filter */
  kalman_filter = {KF_I2C_MAX_DT, KF_I2C_VAR_ACCEL};

  auto i2c = factory.OpenI2Cbaro(config, *this);
  java_sensor = new Java::GlobalCloseable(i2c);

  return true;
#else
  return false;
#endif
}

inline bool
DeviceDescriptor::OpenNunchuck()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  joy_state_x = joy_state_y = 0;

  auto nunchuk = factory.OpenNunchuck(config, *this);
  java_sensor = new Java::GlobalCloseable(nunchuk);
  return true;
#else
  return false;
#endif
}

inline bool
DeviceDescriptor::OpenVoltage()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  voltage_offset = config.sensor_offset;
  voltage_factor = config.sensor_factor;

  for (auto &i : voltage_filter)
    i.Reset();
  temperature_filter.Reset();

  auto voltage = factory.OpenVoltage(*this);
  java_sensor = new Java::GlobalCloseable(voltage);
  return true;
#else
  return false;
#endif
}

inline bool
DeviceDescriptor::OpenGliderLink()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  java_sensor = new Java::GlobalCloseable(factory.OpenGliderLink(*this));
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

  java_sensor = new Java::GlobalCloseable(factory.OpenBluetoothSensor(config, *this));
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
    const std::lock_guard lock{mutex};
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
    port = factory.OpenPort(config, this, *this);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    const auto e = std::current_exception();

    char name_buffer[64];
    const char *name = config.GetPortName(name_buffer, 64);

    LogError(e, name);

    const auto msg = GetFullMessage(e);
    if (!msg.empty()) {
      LockSetErrorMessage(msg.c_str());

      StaticString<256> _msg;
      _msg.Format("%s: %s (%s)", _("Unable to open port"), name, msg.c_str());
      env.SetErrorMessage(_msg);
    }

    return false;
  }

  if (port == nullptr) {
    char name_buffer[64];
    const char *name = config.GetPortName(name_buffer, 64);

    StaticString<256> msg;
    msg.Format("%s: %s.", _("Unable to open port"), name);
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
  const auto e = std::current_exception();
  LogError(e);

  const auto msg = GetFullMessage(e);

  if (!msg.empty()) {
    LockSetErrorMessage(msg.c_str());
    env.SetErrorMessage(msg.c_str());
  }

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

  char buffer[64];
  LogFormat("Opening device: %s", config.GetPortName(buffer, 64));

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
    const std::lock_guard lock{mutex};
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
DeviceDescriptor::SlowReopen(OperationEnvironment &env)
{
  assert(InMainThread());
  assert(!IsBorrowed());

  Close();
  env.Sleep(std::chrono::seconds(5));
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

  char buffer[64];
  LogFormat("Reconnecting to device: %s", config.GetPortName(buffer, 64));

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

const char *
DeviceDescriptor::GetDisplayName() const noexcept
{
  return driver != nullptr
    ? driver->display_name
    : nullptr;
}

bool
DeviceDescriptor::IsDriver(const char *name) const noexcept
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
     blackboard.IsFLARM(index));
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

    if (StringIsEqual(driver->name, "LX") && device != nullptr) {
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
  const std::lock_guard lock{blackboard.mutex};
  return blackboard.RealState(index).alive;
}

TimeStamp
DeviceDescriptor::GetClock() const noexcept
{
  const std::lock_guard lock{blackboard.mutex};
  const NMEAInfo &basic = blackboard.RealState(index);
  return basic.clock;
}

NMEAInfo
DeviceDescriptor::GetData() const noexcept
{
  const std::lock_guard lock{blackboard.mutex};
  return blackboard.RealState(index);
}

DeviceDataEditor
DeviceDescriptor::BeginEdit() noexcept
{
  return {blackboard, index};
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
DeviceDescriptor::PutCrewMass(double crew_mass, OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device ||
      settings_sent.ComparePolarPilotWeight(crew_mass))
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    const ScopeReturnDevice restore(*this, env);
    if (!device->PutCrewMass(crew_mass, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutCrewMass() failed");
    return false;
  }

  settings_sent.polar_pilot_weight = crew_mass;
  settings_sent.polar_pilot_weight_available.Update(GetClock());

  return true;
}

bool
DeviceDescriptor::PutEmptyMass(double empty_mass, OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device ||
      settings_sent.ComparePolarEmptyWeight(empty_mass))
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    const ScopeReturnDevice restore(*this, env);
    if (!device->PutEmptyMass(empty_mass, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutEmptyMass() failed");
    return false;
  }

  settings_sent.polar_empty_weight = empty_mass;
  settings_sent.polar_empty_weight_available.Update(GetClock());

  return true;
}

bool
DeviceDescriptor::PutPolar(const GlidePolar &polar,
                           OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device ||
      config.polar_sync != DeviceConfig::PolarSync::SEND)
    return true;

  if (!Borrow())
    return false;

  try {
    const ScopeReturnDevice restore(*this, env);
    if (!device->PutPolar(polar, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutPolar() failed");
    return false;
  }

  return true;
}

bool
DeviceDescriptor::PutTarget(const GeoPoint &location, const char *name,
                            std::optional<double> elevation,
                            OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    return false;

  try {
    const ScopeReturnDevice restore(*this, env);
    if (!device->PutTarget(location, name, elevation, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutTarget() failed");
    return false;
  }

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
                                     const char *name,
                                     OperationEnvironment &env) noexcept
{
  assert(InMainThread());
  assert(frequency.IsDefined());

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
DeviceDescriptor::ExchangeRadioFrequencies(OperationEnvironment &env,
                                           NMEAInfo &info) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    return device->ExchangeRadioFrequencies(env, info);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "ExchangeRadioFrequencies() failed");
    return false;
  }
}

bool
DeviceDescriptor::PutStandbyFrequency(RadioFrequency frequency,
                                      const char *name,
                                      OperationEnvironment &env) noexcept
{
  assert(InMainThread());
  assert(frequency.IsDefined());

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
DeviceDescriptor::PutTransponderCode(TransponderCode code,
                                     OperationEnvironment &env) noexcept
{
  assert(InMainThread());
  assert(code.IsDefined());

  if (device == nullptr || !config.sync_to_device)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    return device->PutTransponderCode(code, env);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutTransponderCode() failed");
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

bool
DeviceDescriptor::PutElevation(int elevation, OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device ||
      settings_sent.CompareElevation(elevation))
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    if (!device->PutElevation(elevation, env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "PutElevation() failed");
    return false;
  }

  settings_sent.elevation = elevation;
  settings_sent.elevation_available.Update(GetClock());

  return true;
}

bool
DeviceDescriptor::RequestElevation(OperationEnvironment &env) noexcept
{
  assert(InMainThread());

  if (device == nullptr)
    return true;

  if (!Borrow())
    /* TODO: postpone until the borrowed device has been returned */
    return false;

  try {
    ScopeReturnDevice restore(*this, env);
    if (!device->RequestElevation(env))
      return false;
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    LogError(std::current_exception(), "RequestElevation() failed");
    return false;
  }

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

bool
DeviceDescriptor::Declare(const struct Declaration &declaration,
                          const Waypoint *home,
                          OperationEnvironment &env)
{
  assert(borrowed);
  assert(port != nullptr);
  assert(driver != nullptr);
  assert(device != nullptr);

  /* always declare to the primary device first */
  StaticString<60> text;
  text.Format("%s: %s.", _("Sending declaration"), driver->display_name);
  env.SetText(text);

  bool result = device->Declare(declaration, home, env);

  if (driver->HasPassThrough() && second_device != nullptr) {
    /* explicitly configured passthrough device (e.g. FLARM behind
       LXNAV vario): enable passthrough and declare to it */
    text.Format("%s: %s.", _("Sending declaration"),
                second_driver->display_name);
    env.SetText(text);

    /* The primary declaration (above) may have stopped the Rx
       thread.  Restart it so EnablePassThrough() operates with the
       Rx thread consuming data — this is required for the vario to
       properly transition to DIRECT mode and for the serial buffers
       to be drained while the mode switch settles. */
    port->StartRxThread();

    device->EnablePassThrough(env);

    /* Stop the Rx thread and flush all stale data from the serial
       buffers.  Then send a FLARM version request as a "ping" to
       verify the passthrough is working bidirectionally before
       attempting the actual declaration.  This also gives the vario
       firmware additional time to complete the DIRECT mode
       transition. */
    port->StopRxThread();
    port->FullFlush(env, std::chrono::milliseconds(50),
                    std::chrono::milliseconds(500));
    PortWriteNMEA(*port, "PFLAV,R", env);
    try {
      port->ExpectString("PFLAV,A",  env, std::chrono::seconds(2));
    } catch (...) {
      /* FLARM did not respond to the ping — passthrough may not be
         working; continue anyway and let the declaration fail
         gracefully */
    }
    port->StartRxThread();

    result |= second_device->Declare(declaration, home, env);
  } else {
    /* no explicit passthrough device; try the "muxed FLARM" hack
       if FLARM sentences were detected in the NMEA stream */
    const bool flarm = blackboard.IsFLARM(index) &&
      !IsDriver("FLARM");

    if (flarm) {
      text.Format("%s: FLARM.", _("Sending declaration"));
      env.SetText(text);

      result |= DeclareToFLARM(declaration, *port, *driver, device,
                                home, env);
    }
  }

  return result;
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
    text.Format("%s: %s.", _("Reading flight list"),
                second_driver->display_name);
    env.SetText(text);

    device->EnablePassThrough(env);
    return second_device->ReadFlightList(flight_list, env);
  } else {
    text.Format("%s: %s.", _("Reading flight list"), driver->display_name);
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
    text.Format("%s: %s.", _("Downloading flight log"),
                second_driver->display_name);
    env.SetText(text);

    device->EnablePassThrough(env);
    return second_device->DownloadFlight(flight, path, env);
  } else {
    text.Format("%s: %s.", _("Downloading flight log"),
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
  const std::lock_guard lock{mutex};

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

inline void
DeviceDescriptor::LockSetErrorMessage(const char *msg) noexcept
{
    const std::lock_guard lock{mutex};
    error_message = msg;
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
    char buffer[64];
    LogFormat("Device error on %s: %s",
              config.GetPortName(buffer, 64), msg);
  }

  LockSetErrorMessage(msg);

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
    auto basic = blackboard.LockGetDeviceDataUpdateClock(index);

    const ExternalSettings old_settings = basic.settings;

    /* call Device::DataReceived() without holding
       DeviceBlackboard::mutex to avoid blocking all other threads */
    if (device->DataReceived(s, basic)) {
      if (!config.sync_from_device)
        basic.settings = old_settings;

      blackboard.LockSetDeviceDataScheduleMerge(index, basic);
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
  if (nmea_logger != nullptr) {
    /* Skip logging high-frequency LXWP2 sentences */
    if (!StringStartsWith(line, "$LXWP2,"))
      nmea_logger->Log(line);
  }

  if (dispatcher != nullptr)
    dispatcher->LineReceived(line);

  const auto e = BeginEdit();
  e->UpdateClock();
  ParseNMEA(line, *e);
  e.Commit();

  return true;
}
