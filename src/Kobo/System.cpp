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
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "OS/Sleep.h"
#include "Util/StaticString.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef KOBO

static bool
InsMod(const char *path)
{
  NarrowString<256> buffer;
  buffer.Format("/sbin/insmod '%s'", path);
  return system(buffer) == 0;
}

static bool
RmMod(const char *name)
{
  NarrowString<256> buffer;
  buffer.Format("/sbin/rmmod '%s'", name);
  return system(buffer) == 0;
}

#endif

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
IsKoboWifiOn()
{
#ifdef KOBO
  return Directory::Exists("/sys/class/net/eth0");
#else
  return false;
#endif
}

bool
KoboWifiOn()
{
#ifdef KOBO
  InsMod("/drivers/ntx508/wifi/sdio_wifi_pwr.ko");
  InsMod("/drivers/ntx508/wifi/dhd.ko");

  Sleep(2000);

  system("/sbin/ifconfig eth0 up");
  system("/bin/wlarm_le -i eth0 up");
  system("/bin/wpa_supplicant -s -i eth0 -c /etc/wpa_supplicant/wpa_supplicant.conf -C /var/run/wpa_supplicant -B");

  Sleep(2000);

  system("/sbin/udhcpc -S -i eth0 -s /etc/udhcpc.d/default.script -t15 -T10 -A3 -f -q &");

  return true;
#else
  return false;
#endif
}

bool
KoboWifiOff()
{
#ifdef KOBO
  system("/usr/bin/killall wpa_supplicant udhcpc");
  system("/bin/wlarm_le -i eth0 down");
  system("/sbin/ifconfig eth0 down");

  RmMod("dhd");
  RmMod("sdio_wifi_pwr");

  return true;
#else
  return false;
#endif
}

void
KoboExecNickel()
{
#ifdef KOBO
  /* our "rcS" will call the original Kobo "rcS" if start_nickel
     exists */
  mkdir("/mnt/onboard/XCSoarData", 0777);
  mkdir("/mnt/onboard/XCSoarData/kobo", 0777);
  File::CreateExclusive("/mnt/onboard/XCSoarData/kobo/start_nickel");

  /* unfortunately, a bug in the Kobo applications forces us to reboot
     the Kobo at this point */
  KoboReboot();
#endif
}

void
KoboRunXCSoar()
{
#ifdef KOBO
  char buffer[256];
  const char *cmd = buffer;

  if (readlink("/proc/self/exe", buffer, sizeof(buffer)) > 0)
    ReplaceBaseName(buffer, "xcsoar");
  else
    cmd = "/mnt/onboard/XCSoar/xcsoar";

  system(cmd);
#endif
}

void
KoboRunTelnetd()
{
#ifdef KOBO
  system("/usr/sbin/telnetd -l /bin/sh");
#endif
}
