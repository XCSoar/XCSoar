// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MultipleDevices.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Descriptor.hpp"
#include "Dispatcher.hpp"

#include <algorithm> // for std::any_of()

MultipleDevices::MultipleDevices(DeviceBlackboard &blackboard,
                                 NMEALogger *nmea_logger,
                                 DeviceFactory &factory) noexcept
{
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDispatcher *dispatcher = dispatchers[i] =
      new DeviceDispatcher(*this, i);

    devices[i] = new DeviceDescriptor(blackboard, nmea_logger,
                                      factory, i, this);
    devices[i]->SetDispatcher(dispatcher);
  }
}

MultipleDevices::~MultipleDevices() noexcept
{
  for (DeviceDescriptor *i : devices)
    delete i;

  for (DeviceDispatcher *i : dispatchers)
    delete i;
}

void
MultipleDevices::Tick() noexcept
{
  for (DeviceDescriptor *i : devices)
    i->OnSysTicker();
}

void
MultipleDevices::Open(OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->Open(env);
}

void
MultipleDevices::Close() noexcept
{
  for (DeviceDescriptor *i : devices)
    i->Close();
}

void
MultipleDevices::AutoReopen(OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->AutoReopen(env);
}

bool
MultipleDevices::HasVega() const noexcept
{
  return std::any_of(devices.begin(), devices.end(),
                     [](const auto *d) { return d->IsVega(); });
}

void
MultipleDevices::VegaWriteNMEA(const TCHAR *text,
                               OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    if (i->IsVega())
      i->WriteNMEA(text, env);
}

void
MultipleDevices::PutMacCready(double mac_cready,
                              OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutMacCready(mac_cready, env);
}

void
MultipleDevices::PutBugs(double bugs, OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutBugs(bugs, env);
}

void
MultipleDevices::PutBallast(double fraction, double overload,
                            OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutBallast(fraction, overload, env);
}

void
MultipleDevices::PutVolume(unsigned volume, OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutVolume(volume, env);
}

void
MultipleDevices::PutPilotEvent(OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutPilotEvent(env);
}

void
MultipleDevices::PutActiveFrequency(RadioFrequency frequency,
                                    const TCHAR *name,
                                    OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutActiveFrequency(frequency, name, env);
}

void
MultipleDevices::PutStandbyFrequency(RadioFrequency frequency,
                                     const TCHAR *name,
                                     OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutStandbyFrequency(frequency, name, env);
}

void
MultipleDevices::PutTransponderCode(TransponderCode code,
                                    OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutTransponderCode(code, env);
}

void
MultipleDevices::PutQNH(AtmosphericPressure pres,
                        OperationEnvironment &env) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->PutQNH(pres, env);
}

void
MultipleDevices::NotifySensorUpdate(const MoreData &basic) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->OnSensorUpdate(basic);
}

void
MultipleDevices::NotifyCalculatedUpdate(const MoreData &basic,
                                        const DerivedInfo &calculated) noexcept
{
  for (DeviceDescriptor *i : devices)
    i->OnCalculatedUpdate(basic, calculated);
}

void
MultipleDevices::AddPortListener(PortListener &listener) noexcept
{
  const std::lock_guard lock{listeners_mutex};
  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) == listeners.end());
  listeners.push_back(&listener);
}

void
MultipleDevices::RemovePortListener(PortListener &listener) noexcept
{
  const std::lock_guard lock{listeners_mutex};
  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) != listeners.end());
  listeners.remove(&listener);
}

void
MultipleDevices::PortStateChanged() noexcept
{
  const std::lock_guard lock{listeners_mutex};

  for (auto *listener : listeners)
    listener->PortStateChanged();
}

void
MultipleDevices::PortError(const char *msg) noexcept
{
  const std::lock_guard lock{listeners_mutex};

  for (auto *listener : listeners)
    listener->PortError(msg);
}
