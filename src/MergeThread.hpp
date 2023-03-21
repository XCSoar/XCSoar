// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/WorkerThread.hpp"
#include "Computer/BasicComputer.hpp"
#include "FLARM/FlarmComputer.hpp"
#include "NMEA/MoreData.hpp"

class DeviceBlackboard;

/**
 * The MergeThread collects new data from the DeviceBlackboard, merges
 * it and runs a number of cheap calculations.
 */
class MergeThread final : public WorkerThread {
  DeviceBlackboard &device_blackboard;

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
  MergeThread(DeviceBlackboard &_device_blackboard);

  /**
   * This method is called during XCSoar startup, for the initial run
   * of the MergeThread.
   */
  void FirstRun() {
    assert(!IsDefined());

    Process();
  }

  /**
   * Throws on error.
   */
  void Start(bool suspended=false) {
    WorkerThread::Start(suspended);
    SetLowPriority();
  }

private:
  void Process();

protected:
  void Tick() noexcept override;
};
