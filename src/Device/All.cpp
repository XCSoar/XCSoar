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

#include "Device/All.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/device.hpp"
#include "Thread/Mutex.hpp"
#include "Simulator.hpp"

#include <assert.h>

void
devTick()
{
  int i;

  for (i = 0; i < NUMDEV; i++) {
    DeviceDescriptor *d = &DeviceList[i];
    d->OnSysTicker();
  }
}

void
AllDevicesPutMacCready(double MacCready)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutMacCready(MacCready);
}

void
AllDevicesPutBugs(double bugs)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutBugs(bugs);
}

void
AllDevicesPutBallast(double ballast)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutBallast(ballast);
}

void
AllDevicesPutVolume(int volume)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutVolume(volume);
}

void
AllDevicesPutActiveFrequency(double frequency)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutActiveFrequency(frequency);
}

void
AllDevicesPutStandbyFrequency(double frequency)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutStandbyFrequency(frequency);
}

void
AllDevicesPutQNH(const AtmosphericPressure& pres)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutQNH(pres);
}

void
AllDevicesPutVoice(const TCHAR *sentence)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].PutVoice(sentence);
}

void
AllDevicesLinkTimeout()
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    DeviceList[i].LinkTimeout();
}

bool
AllDevicesIsDeclaring()
{
  assert(!is_simulator());

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceList[i].IsDeclaring())
      return true;

  return false;
}
