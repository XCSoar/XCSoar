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

#ifndef XCSOAR_DEVICE_ALL_HPP
#define XCSOAR_DEVICE_ALL_HPP

#include "Math/fixed.hpp"
#include "Compiler.h"
#include "RadioFrequency.hpp"

#include <tchar.h>

struct NMEA_INFO;
struct DERIVED_INFO;
class AtmosphericPressure;

void
devTick(const DERIVED_INFO &calculated);

void AllDevicesPutMacCready(fixed MacCready);
void AllDevicesPutBugs(fixed bugs);
void AllDevicesPutBallast(fixed ballast);
void AllDevicesPutVolume(int volume);

void
AllDevicesPutActiveFrequency(RadioFrequency frequency);

void
AllDevicesPutStandbyFrequency(RadioFrequency frequency);

void
AllDevicesPutQNH(const AtmosphericPressure &pres,
                 const DERIVED_INFO &calculated);

void AllDevicesPutVoice(const TCHAR *sentence);

/**
 * Is any device currently declaring a task?
 */
gcc_pure
bool
AllDevicesIsBusy();

#endif
