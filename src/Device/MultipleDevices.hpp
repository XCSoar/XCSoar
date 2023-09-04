// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

class DeviceBlackboard;
class NMEALogger;
class DeviceFactory;
class DeviceDescriptor;
class DeviceDispatcher;
struct MoreData;
struct DerivedInfo;
class AtmosphericPressure;
class RadioFrequency;
class TransponderCode;
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
  MultipleDevices(DeviceBlackboard &blackboard,
                  NMEALogger *nmea_logger,
                  DeviceFactory &factory) noexcept;
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

  [[gnu::pure]]
  bool HasVega() const noexcept;

  void VegaWriteNMEA(const TCHAR *text, OperationEnvironment &env) noexcept;

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
  void PutTransponderCode(TransponderCode code, OperationEnvironment &env) noexcept;
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
