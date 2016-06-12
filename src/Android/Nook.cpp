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

#include "Nook.hpp"
#include "OS/FileUtil.hpp"
#include "OS/Sleep.h"

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
