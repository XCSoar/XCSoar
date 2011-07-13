/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "MergeThread.hpp"
#include "DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "NMEA/MoreData.hpp"

MergeThread::MergeThread(DeviceBlackboard &_device_blackboard)
  :WorkerThread(150, 50, 20),
   device_blackboard(_device_blackboard)
{
  last_fix.Reset();
  last_any.Reset();
}

void
MergeThread::Tick()
{
  ScopeLock protect(device_blackboard.mutex);

  device_blackboard.Merge();

  const MoreData &basic = device_blackboard.Basic();
  const SETTINGS_COMPUTER &settings_computer =
    device_blackboard.SettingsComputer();

  computer.Fill(device_blackboard.SetMoreData(), settings_computer);
  computer.Compute(device_blackboard.SetMoreData(), last_fix,
                   device_blackboard.Calculated(), settings_computer);

  flarm_computer.Process(device_blackboard.SetBasic().flarm,
                         last_fix.flarm, basic);

  if (last_any.LocationAvailable != basic.LocationAvailable)
    // trigger update if gps has become available or dropped out
    TriggerGPSUpdate();

  TriggerVarioUpdate();

  /* update last_any in every iteration */
  last_any = basic;

  /* update last_fix only when a new GPS fix was received */
  if (basic.Time != last_fix.Time ||
      basic.LocationAvailable != last_fix.LocationAvailable)
    last_fix = basic;
}
