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

#include "MultipleDevices.hpp"
#include "Descriptor.hpp"
#include "Dispatcher.hpp"

MultipleDevices::MultipleDevices(boost::asio::io_service &io_service)
{
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDispatcher *dispatcher = dispatchers[i] =
      new DeviceDispatcher(*this, i);

    devices[i] = new DeviceDescriptor(io_service, i, this);
    devices[i]->SetDispatcher(dispatcher);
  }
}

MultipleDevices::~MultipleDevices()
{
  for (DeviceDescriptor *i : devices)
    delete i;

  for (DeviceDispatcher *i : dispatchers)
    delete i;
}

void
MultipleDevices::Tick()
{
  for (DeviceDescriptor *i : devices)
    i->OnSysTicker();
}

void
MultipleDevices::AutoReopen(OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->AutoReopen(env);
}

void
MultipleDevices::PutMacCready(double mac_cready, OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutMacCready(mac_cready, env);
}

void
MultipleDevices::PutBugs(double bugs, OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutBugs(bugs, env);
}

void
MultipleDevices::PutBallast(double fraction, double overload,
                            OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutBallast(fraction, overload, env);
}

void
MultipleDevices::PutVolume(unsigned volume, OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutVolume(volume, env);
}

void
MultipleDevices::PutActiveFrequency(RadioFrequency frequency,
                                    const TCHAR *name,
                                    OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutActiveFrequency(frequency, name, env);
}

void
MultipleDevices::PutStandbyFrequency(RadioFrequency frequency,
                                     const TCHAR *name,
                                     OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutStandbyFrequency(frequency, name, env);
}

void
MultipleDevices::PutQNH(const AtmosphericPressure &pres,
                        OperationEnvironment &env)
{
  for (DeviceDescriptor *i : devices)
    i->PutQNH(pres, env);
}

void
MultipleDevices::NotifySensorUpdate(const MoreData &basic)
{
  for (DeviceDescriptor *i : devices)
    i->OnSensorUpdate(basic);
}

void
MultipleDevices::NotifyCalculatedUpdate(const MoreData &basic,
                                        const DerivedInfo &calculated)
{
  for (DeviceDescriptor *i : devices)
    i->OnCalculatedUpdate(basic, calculated);
}

void
MultipleDevices::AddPortListener(PortListener &listener)
{
  const ScopeLock protect(listeners_mutex);
  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) == listeners.end());
  listeners.push_back(&listener);
}

void
MultipleDevices::RemovePortListener(PortListener &listener)
{
  const ScopeLock protect(listeners_mutex);
  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) != listeners.end());
  listeners.remove(&listener);
}

void
MultipleDevices::PortStateChanged()
{
  const ScopeLock protect(listeners_mutex);

  for (auto *listener : listeners)
    listener->PortStateChanged();
}

void
MultipleDevices::PortError(const char *msg)
{
  const ScopeLock protect(listeners_mutex);

  for (auto *listener : listeners)
    listener->PortError(msg);
}
