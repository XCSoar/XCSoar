/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

/** \file
 *
 * This library manages the list of configured devices.
 */

#pragma once

#include "Features.hpp"
#include "Device/Port/Listener.hpp"
#include "thread/Mutex.hxx"

#include <array>
#include <list>
#include <tchar.h>

namespace Cares { class Channel; }
class EventLoop;
class DeviceDescriptor;
class DeviceDispatcher;
struct MoreData;
struct DerivedInfo;
class AtmosphericPressure;
class RadioFrequency;
class OperationEnvironment;

/**
 * Container for all (configured) devices.
 */
class MultipleDevices final : PortListener {
  std::array<DeviceDescriptor *, NUMDEV> devices;
  std::array<DeviceDispatcher *, NUMDEV> dispatchers;

  Mutex listeners_mutex;
  std::list<PortListener *> listeners;

public:
  MultipleDevices(EventLoop &event_loop, Cares::Channel &cares) noexcept;
  ~MultipleDevices() noexcept;

  DeviceDescriptor &operator[](unsigned i) const noexcept {
    return *devices[i];
  }

  typedef typename std::array<DeviceDescriptor *, NUMDEV>::const_iterator const_iterator;

  const_iterator begin() noexcept {
    return devices.begin();
  }

  const_iterator end() noexcept {
    return devices.end();
  }

  /**
   * Invoke Device::OnSysTicker() on all devices.
   */
  void Tick() noexcept;

  void Open(OperationEnvironment &env) noexcept;
  void Close() noexcept;
  void AutoReopen(OperationEnvironment &env) noexcept;
  void PutMacCready(double mac_cready, OperationEnvironment &env) noexcept;
  void PutBugs(double bugs, OperationEnvironment &env) noexcept;
  void PutBallast(double fraction, double overload,
                  OperationEnvironment &env) noexcept;
  void PutVolume(unsigned volume, OperationEnvironment &env) noexcept;
  void PutPilotEvent(OperationEnvironment &env) noexcept;
  void PutActiveFrequency(RadioFrequency frequency, const TCHAR *name,
                          OperationEnvironment &env) noexcept;
  void PutStandbyFrequency(RadioFrequency frequency, const TCHAR *name,
                           OperationEnvironment &env) noexcept;
  void PutQNH(AtmosphericPressure pres, OperationEnvironment &env) noexcept;
  void NotifySensorUpdate(const MoreData &basic) noexcept;
  void NotifyCalculatedUpdate(const MoreData &basic,
                              const DerivedInfo &calculated) noexcept;

  void AddPortListener(PortListener &listener) noexcept;
  void RemovePortListener(PortListener &listener) noexcept;

private:
  /* virtual methods from class PortListener */
  void PortStateChanged() noexcept override;
  void PortError(const char *msg) noexcept override;
};
