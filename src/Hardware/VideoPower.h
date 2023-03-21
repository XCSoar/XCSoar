// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <windef.h>

#define SETPOWERMANAGEMENT   6147
#define GETPOWERMANAGEMENT   6148

enum VIDEO_POWER_STATE {
  VideoPowerOn = 1,
  VideoPowerStandBy,
  VideoPowerSuspend,
  VideoPowerOff
};

struct VIDEO_POWER_MANAGEMENT {
  ULONG Length;
  ULONG DPMSVersion;
  ULONG PowerState;
};
