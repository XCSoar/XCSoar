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
#include "OS/Process.hpp"
#include "OS/Sleep.h"
#include "Util/StaticString.hpp"

#include <unistd.h>
#include <sys/stat.h>

#ifdef KOBO

static bool
InsMod(const char *path)
{
  return Run("/sbin/insmod", path);
}

static bool
RmMod(const char *name)
{
  return Run("/sbin/rmmod", name);
}

/**
 * Determine the location of the current program, and build a path to
 * another program in the same directory.
 */
static bool
SiblingPath(const char *name, char *buffer, size_t size)
{
  if (readlink("/proc/self/exe", buffer, size) <= 0)
    return false;

  ReplaceBaseName(buffer, name);
  return true;
}

#endif

bool
KoboReboot()
{
#ifdef KOBO
  return Run("/sbin/reboot");
#else
  return false;
#endif
}

bool
KoboPowerOff()
{
#ifdef KOBO
  return Run("/sbin/poweroff");
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

  Run("/sbin/ifconfig", "eth0", "up");
  Run("/bin/wlarm_le", "-i", "eth0", "up");
  Run("/bin/wpa_supplicant", "-i", "eth0",
      "-c", "/etc/wpa_supplicant/wpa_supplicant.conf",
      "-C", "/var/run/wpa_supplicant", "-B");

  Sleep(2000);

  Start("/sbin/udhcpc", "-S", "-i", "eth0",
        "-s", "/etc/udhcpc.d/default.script",
        "-t15", "-T10", "-A3", "-f", "-q");

  return true;
#else
  return false;
#endif
}

bool
KoboWifiOff()
{
#ifdef KOBO
  Run("/usr/bin/killall", "wpa_supplicant", "udhcpc");
  Run("/bin/wlarm_le", "-i", "eth0", "down");
  Run("/sbin/ifconfig", "eth0", "down");

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

  if (!SiblingPath("xcsoar", buffer, sizeof(buffer)))
    cmd = "/mnt/onboard/XCSoar/xcsoar";

  Run(cmd);
#endif
}

void
KoboRunTelnetd()
{
#ifdef KOBO
  Run("/usr/sbin/telnetd", "-l", "/bin/sh");
#endif
}
