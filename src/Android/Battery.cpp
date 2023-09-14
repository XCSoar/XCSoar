// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Hardware/PowerGlobal.hpp"
#include "Hardware/PowerInfo.hpp"
#include "org_xcsoar_BatteryReceiver.h"
#include "util/Compiler.h"

gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_BatteryReceiver_setBatteryPercent([[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls,
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
