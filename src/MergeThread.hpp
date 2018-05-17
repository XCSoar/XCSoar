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

#ifndef XCSOAR_MERGE_THREAD_HPP
#define XCSOAR_MERGE_THREAD_HPP

#include "Thread/WorkerThread.hpp"
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

  bool Start(bool suspended=false) {
    if (!WorkerThread::Start(suspended))
      return false;

    SetLowPriority();
    return true;
  }

private:
  void Process();

protected:
  virtual void Tick();
};

#endif
