// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Nook.hpp"
#include "system/FileUtil.hpp"
#include "system/Sleep.h"

#include <stdlib.h>

static char cmd_host[] = "su -c 'echo host > /sys/devices/platform/musb_hdrc/mode'";
static char cmd_set_charge_500[] = "su -c 'echo 500000 > /sys/class/regulator/regulator.5/device/force_current'";
static char cmd_set_charge_100[] = "su -c 'echo 100000 > /sys/class/regulator/regulator.5/device/force_current'";

bool
Nook::EnterFastMode()
{
  File::WriteExisting(Path("/sys/class/graphics/fb0/epd_refresh"), "0");
  Sleep(1000);
  File::WriteExisting(Path("/sys/class/graphics/fb0/epd_refresh"), "1");
  return File::WriteExisting(Path("/sys/class/graphics/fb0/fmode"), "1");
}

void
Nook::ExitFastMode()
{
  File::WriteExisting(Path("/sys/class/graphics/fb0/fmode"), "0");
  File::WriteExisting(Path("/sys/class/graphics/fb0/epd_refresh"), "0");
  Sleep(500);
  File::WriteExisting(Path("/sys/class/graphics/fb0/epd_refresh"), "1");
}

void
Nook::InitUsb()
{
  system(cmd_host);
  Sleep(500);

  system(cmd_host);
  Sleep(500);
}

void
Nook::SetCharge500()
{
  system(cmd_set_charge_500);
}

void
Nook::SetCharge100()
{
  system(cmd_set_charge_100);
}
