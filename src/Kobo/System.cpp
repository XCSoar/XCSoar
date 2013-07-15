/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "System.hpp"
#include "OS/PathName.hpp"

#include <stdlib.h>
#include <unistd.h>

bool
KoboReboot()
{
#ifdef KOBO
  return system("/sbin/reboot") == 0;
#else
  return false;
#endif
}

bool
KoboPowerOff()
{
#ifdef KOBO
  return system("/sbin/poweroff") == 0;
#else
  return false;
#endif
}

bool
KoboWifiOn()
{
#ifdef KOBO
  return system("/mnt/onboard/XCSoar/wifiup.sh") == 0;
#else
  return false;
#endif
}

bool
KoboWifiOff()
{
#ifdef KOBO
  return system("/mnt/onboard/XCSoar/wifidown.sh") == 0;
#else
  return false;
#endif
}

void
KoboExecNickel()
{
#ifdef KOBO
  const char *cmd = "/mnt/onboard/XCSoar/restartnickel.sh";
  execl(cmd, cmd, nullptr);
#endif
}

void
KoboExecXCSoar()
{
#ifdef KOBO
  char buffer[256];
  const char *cmd = buffer;

  if (readlink("/proc/self/exe", buffer, sizeof(buffer)) > 0)
    ReplaceBaseName(buffer, "xcsoar");
  else
    cmd = "/mnt/onboard/XCSoar/xcsoar";

  execl(cmd, cmd, nullptr);
#endif
}
