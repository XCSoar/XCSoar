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

/** \file
 *
 * This library manages the list of configured devices.
 */

#ifndef XCSOAR_DEVICE_LIST_HPP
#define XCSOAR_DEVICE_LIST_HPP

#include "Features.hpp"
#include "Device/Port/Listener.hpp"
#include "Thread/Mutex.hpp"

#include <array>
#include <list>

#include <tchar.h>

namespace boost { namespace asio { class io_service; }}

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
  MultipleDevices(boost::asio::io_service &io_service);
  ~MultipleDevices();

  DeviceDescriptor &operator[](unsigned i) const {
    return *devices[i];
  }

  typedef typename std::array<DeviceDescriptor *, NUMDEV>::const_iterator const_iterator;

  const_iterator begin() {
    return devices.begin();
  }

  const_iterator end() {
    return devices.end();
  }

  /**
   * Invoke Device::OnSysTicker() on all devices.
   */
  void Tick();

  void AutoReopen(OperationEnvironment &env);
  void PutMacCready(double mac_cready, OperationEnvironment &env);
  void PutBugs(double bugs, OperationEnvironment &env);
  void PutBallast(double fraction, double overload, OperationEnvironment &env);
  void PutVolume(unsigned volume, OperationEnvironment &env);
  void PutActiveFrequency(RadioFrequency frequency, const TCHAR *name,
                          OperationEnvironment &env);
  void PutStandbyFrequency(RadioFrequency frequency, const TCHAR *name,
                           OperationEnvironment &env);
  void PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env);
  void NotifySensorUpdate(const MoreData &basic);
  void NotifyCalculatedUpdate(const MoreData &basic,
                              const DerivedInfo &calculated);

  void AddPortListener(PortListener &listener);
  void RemovePortListener(PortListener &listener);

private:
  /* virtual methods from class PortListener */
  void PortStateChanged() override;
  void PortError(const char *msg) override;
};

#endif
