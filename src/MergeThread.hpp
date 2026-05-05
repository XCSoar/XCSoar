// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/WorkerThread.hpp"
#include "Computer/BasicComputer.hpp"
#include "FLARM/Computer.hpp"
#include "NMEA/MoreData.hpp"

class DeviceBlackboard;
class MultipleDevices;
class TraceComputer;

/**
 * The MergeThread collects new data from the DeviceBlackboard, merges
 * it and runs a number of cheap calculations.
 */
class MergeThread final : public WorkerThread {
  DeviceBlackboard &device_blackboard;

  MultipleDevices *const devices;

  /**
   * Optional sink for high-rate netto vario (snail trail colouring).
   * Must not be called while #DeviceBlackboard::mutex is locked.
   */
  TraceComputer *trail_vario_sink = nullptr;

  /**
   * The previous values at the time of the last GPS fix (last
   * LocationAvailable modification).
   */
  MoreData last_fix;

  /**
   * The previous values at the time of the last update of any
   * attribute (last Connected modification).
   */
  MoreData last_any;

  BasicComputer computer;
  FlarmComputer flarm_computer;

public:
  MergeThread(DeviceBlackboard &_device_blackboard,
              MultipleDevices *_devices,
              TraceComputer *_trail_vario_sink=nullptr) noexcept;

  /**
   * This method is called during XCSoar startup, for the initial run
   * of the MergeThread.
   */
  void FirstRun() noexcept {
    assert(!IsDefined());

    ProcessUnlocked();
  }

  /**
   * Run one merge cycle synchronously from the main thread while this
   * worker thread is suspended.  Omits UI/device triggers when false is
   * passed (bulk replay seek).
   */
  void RunMergeCycle(bool notify_triggers) noexcept;

private:
  void ProcessUnlocked() noexcept;

public:
  /**
   * Throws on error.
   */
  void Start(bool suspended=false) {
    WorkerThread::Start(suspended);
    SetLowPriority();
  }

protected:
  void Tick() noexcept override;
};
