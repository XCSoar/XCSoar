/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Hardware/PowerGlobal.hpp"
#include "Hardware/PowerInfo.hpp"
#include "org_xcsoar_BatteryReceiver.h"
#include "util/Compiler.h"

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_BatteryReceiver_setBatteryPercent(JNIEnv *env, jclass cls,
                                                  jint value, jint plugged)
{
  auto &info = Power::global_info;
  auto &battery = info.battery;
  auto &external = info.external;

  battery.remaining_percent = value;

  switch (plugged) {
  case 0:
    external.status = Power::ExternalInfo::Status::OFF;
    break;

  default:
    external.status = Power::ExternalInfo::Status::ON;
    break;
  }
}
