/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Thread/Mutex.hpp"
#include "Util/StringAPI.hxx"
#include "Util/ConvertString.hpp"
#include "Util/Exception.hxx"
#include "Logger/NMEALogger.hpp"
#include "Language/Language.hpp"
#include "Operation/Operation.hpp"
#include "OS/Path.hpp"
#include "../Simulator.hpp"
#include "Input/InputQueue.hpp"
#include "LogFile.hpp"
#include "Job/Job.hpp"

#ifdef ANDROID
#include "Java/Object.hxx"
#include "Java/Global.hxx"
#include "Android/InternalSensors.hpp"
#include "Android/GliderLink.hpp"
#include "Android/Main.hpp"
#include "Android/Product.hpp"
#include "Android/IOIOHelper.hpp"
#include "Android/BMP085Device.hpp"
#include "Android/I2CbaroDevice.hpp"
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

DeviceDescriptor::DeviceDescriptor(boost::asio::io_service &_io_service,
                                   unsigned _index,
                                   PortListener *_port_listener)
  :io_service(_io_service), index(_index),
   port_listener(_port_listener),
   open_job(nullptr),
   port(nullptr), monitor(nullptr), dispatcher(nullptr),
   driver(nullptr), device(nullptr), second_device(nullptr),
#ifdef HAVE_INTERNAL_GPS
   internal_sensors(nullptr),
#endif
#ifdef ANDROID
   droidsoar_v2(nullptr),
   nunchuck(nullptr),
   voltage(nullptr),
   glider_link(nullptr),
#endif
   n_failures(0u),
   ticker(false), borrowed(false)
{
  config.Clear();

#ifdef ANDROID
  for (unsigned i=0; i<sizeof i2cbaro/sizeof i2cbaro[0]; i++)
    i2cbaro[i] = nullptr;
#endif
}

void
DeviceDescriptor::SetConfig(const DeviceConfig &_config)
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

#ifdef HAVE_INTERNAL_GPS
  if (internal_sensors != nullptr)
    return PortState::READY;
#endif

#ifdef ANDROID
  if (droidsoar_v2 != nullptr)
    return PortState::READY;

  if (i2cbaro[0] != nullptr)
    return PortState::READY;

  if (nunchuck != nullptr)
    return PortState::READY;

  if (voltage != nullptr)
    return PortState::READY;

  if (glider_link != nullptr)
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
DeviceDescriptor::EnableDumpTemporarily(std::chrono::steady_clock::duration duration) noexcept
{
  if (port != nullptr)
    port->EnableTemporarily(duration);
}

bool
DeviceDescriptor::ShouldReopenDriverOnTimeout() const
{
  return driver == nullptr || driver->HasTimeout();
}

void
DeviceDescriptor::CancelAsync()
{
  assert(InMainThread());

  if (!async.IsBusy())
    return;

  assert(open_job != nullptr);

  async.Cancel();

  try {
    async.Wait();
  } catch (...) {
    LogError(std::current_exception());
  }

  delete open_job;
  open_job = nullptr;
}

bool
DeviceDescriptor::OpenOnPort(DumpPort *_port, OperationEnvironment &env)
{
  assert(port == nullptr);
  assert(device == nullptr);
  assert(second_device == nullptr);
  assert(driver != nullptr);
  assert(!ticker);
  assert(!IsBorrowed());

  reopen_clock.Update();

  {
    const ScopeLock lock(device_blackboard->mutex);
    device_blackboard->SetRealState(index).Reset();
    device_blackboard->ScheduleMerge();
  }

  settings_sent.Clear();
  settings_received.Clear();
  was_alive = false;

  port = _port;

  parser.Reset();
  parser.SetReal(!StringIsEqual(driver->name, _T("Condor")));
  if (config.IsDriver(_T("Condor")))
    parser.DisableGeoid();

  if (driver->CreateOnPort != nullptr) {
    Device *new_device = driver->CreateOnPort(config, *port);

    const ScopeLock protect(mutex);
    device = new_device;

    if (driver->HasPassThrough() && config.use_second_device)
      second_device = second_driver->CreateOnPort(config, *port);
  } else
    port->StartRxThread();

  EnableNMEA(env);

  if (env.IsCancelled()) {
    /* the caller is responsible for freeing the port on error */
    port = nullptr;
    delete device;
    device = nullptr;
    delete second_device;
    second_device = nullptr;
    return false;
  }

  return true;
}

bool
DeviceDescriptor::OpenInternalSensors()
{
#ifdef HAVE_INTERNAL_GPS
  if (is_simulator())
    return true;

#ifdef ANDROID
  internal_sensors =
      InternalSensors::create(Java::GetEnv(), context, GetIndex());
  if (internal_sensors) {
    // TODO: Allow user to specify whether they want certain sensors.
    internal_sensors->subscribeToSensor(InternalSensors::TYPE_PRESSURE);
    return true;
  }
#elif defined(__APPLE__)
  internal_sensors = InternalSensors::Create(GetIndex());
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

  if (i2cbaro[0] == nullptr) {
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
                       config.sensor_factor == 0
                       ? DeviceConfig::PressureUse::PITOT_ZERO
                       : DeviceConfig::PressureUse::PITOT,
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
    if (i2cbaro[i] == nullptr) {
      i2cbaro[i] = new I2CbaroDevice(GetIndex(), Java::GetEnv(),
                       ioio_helper->GetHolder(),
                       // needs calibration ?
                       config.sensor_factor == 0 && config.press_use == DeviceConfig::PressureUse::PITOT
                       ? DeviceConfig::PressureUse::PITOT_ZERO
                       : config.press_use,
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
DeviceDescriptor::OpenGliderLink()
{
#ifdef ANDROID
  if (is_simulator())
    return true;

  glider_link = GliderLink::create(Java::GetEnv(), context, GetIndex());

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
    ScopeLock protect(mutex);
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

  reopen_clock.Update();

  Port *port;
  try {
    port = OpenPort(io_service, config, this, *this);
  } catch (...) {
    const auto e = std::current_exception();

    TCHAR name_buffer[64];
    const TCHAR *name = config.GetPortName(name_buffer, 64);

    LogError(e, WideToUTF8Converter(name));

    StaticString<256> msg;

    const UTF8ToWideConverter what(GetFullMessage(e).c_str());
    if (what.IsValid()) {
      ScopeLock protect(mutex);
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
} catch (...) {
  const UTF8ToWideConverter msg(GetFullMessage(std::current_exception()).c_str());
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
  assert(!ticker);
  assert(!IsBorrowed());

  if (is_simulator() || !config.IsAvailable())
    return;

  CancelAsync();

  assert(!IsOccupied());
  assert(open_job == nullptr);

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

#ifdef HAVE_INTERNAL_GPS
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
  delete nunchuck;
  nunchuck = nullptr;

  delete voltage;
  voltage = nullptr;

  delete glider_link;
  glider_link = nullptr;
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

  delete second_device;
  second_device = nullptr;

  Port *old_port = port;
  port = nullptr;
  delete old_port;

  ticker = false;

  {
    const ScopeLock lock(device_blackboard->mutex);
    device_blackboard->SetRealState(index).Reset();
    device_blackboard->ScheduleMerge();
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
      !reopen_clock.CheckUpdate(30000))
    return;

  TCHAR buffer[64];
  LogFormat(_T("Reconnecting to device %s"), config.GetPortName(buffer, 64));

  InputEvents::processGlideComputer(GCE_COMMPORT_RESTART);
  Reopen(env);
}

bool
DeviceDescriptor::EnableNMEA(OperationEnvironment &env)
{
  if (device == nullptr)
    return true;

  bool success = device->EnableNMEA(env);

  if (port != nullptr)
    /* re-enable the NMEA handler if it has been disabled by the
       driver */
    port->StartRxThread();

  return success;
}

const TCHAR *
DeviceDescriptor::GetDisplayName() const
{
  return driver != nullptr
    ? driver->display_name
    : nullptr;
}

bool
DeviceDescriptor::IsDriver(const TCHAR *name) const
{
  return driver != nullptr
    ? StringIsEqual(driver->name, name)
    : false;
}

bool
DeviceDescriptor::CanDeclare() const
{
  return driver != nullptr &&
    (driver->CanDeclare() ||
     device_blackboard->IsFLARM(index));
}

bool
DeviceDescriptor::IsLogger() const
{
  return driver != nullptr && driver->IsLogger();
}

bool
DeviceDescriptor::IsNMEAOut() const
{
  return driver != nullptr && driver->IsNMEAOut();
}

bool
DeviceDescriptor::IsManageable() const
{
  if (driver != nullptr) {
    if (driver->IsManageable())
      return true;

    if (StringIsEqual(driver->name, _T("LX")) && device != nullptr) {
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
    Port *p = port;
    p->Write(line);
    p->Write("\r\n");
  }
}

bool
DeviceDescriptor::WriteNMEA(const char *line, OperationEnvironment &env)
{
  assert(line != nullptr);

  return port != nullptr && PortWriteNMEA(*port, line, env);
}

#ifdef _UNICODE
bool
DeviceDescriptor::WriteNMEA(const TCHAR *line, OperationEnvironment &env)
{
  assert(line != nullptr);

  if (port == nullptr)
    return false;

  char buffer[_tcslen(line) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, line, -1, buffer, sizeof(buffer),
                            nullptr, nullptr) <= 0)
    return false;

  return WriteNMEA(buffer, env);
}
#endif

bool
DeviceDescriptor::PutMacCready(double value, OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == nullptr || settings_sent.CompareMacCready(value) ||
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
DeviceDescriptor::PutBugs(double value, OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == nullptr || settings_sent.CompareBugs(value) ||
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
DeviceDescriptor::PutBallast(double fraction, double overload,
                             OperationEnvironment &env)
{
  assert(InMainThread());

  if (device == nullptr || !config.sync_to_device ||
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

  if (device == nullptr || !config.sync_to_device)
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

  if (device == nullptr || !config.sync_to_device)
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

  if (device == nullptr || !config.sync_to_device)
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

  if (device == nullptr || settings_sent.CompareQNH(value) ||
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
DeviceDescriptor::OnSysTicker()
{
  assert(InMainThread());

  if (port != nullptr && port->GetState() == PortState::FAILED && !IsOccupied())
    Close();

  if (device == nullptr)
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
  assert(open_job != nullptr);

  try {
    async.Wait();
  } catch (...) {
    LogError(std::current_exception());
  }

  delete open_job;
  open_job = nullptr;
}

void
DeviceDescriptor::PortStateChanged()
{
  if (port_listener != nullptr)
    port_listener->PortStateChanged();
}

void
DeviceDescriptor::PortError(const char *msg)
{
  {
    TCHAR buffer[64];
    LogFormat(_T("Error on device %s: %s"),
              config.GetPortName(buffer, 64), msg);
  }

  {
    const UTF8ToWideConverter tmsg(msg);
    if (tmsg.IsValid()) {
      ScopeLock protect(mutex);
      error_message = tmsg;
    }
  }

  if (port_listener != nullptr)
    port_listener->PortError(msg);
}

void
DeviceDescriptor::DataReceived(const void *data, size_t length)
{
  if (monitor != nullptr)
    monitor->DataReceived(data, length);

  // Pass data directly to drivers that use binary data protocols
  if (driver != nullptr && device != nullptr && driver->UsesRawData()) {
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

  if (dispatcher != nullptr)
    dispatcher->LineReceived(line);

  if (ParseLine(line))
    device_blackboard->ScheduleMerge();
}
