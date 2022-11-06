/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "system/FileUtil.hpp"
#include "system/PathName.hpp"
#include "system/Process.hpp"
#include "system/Sleep.h"
#include "util/StaticString.hxx"

#include <unistd.h>
#include <sys/stat.h>

#ifdef KOBO

#include "Model.hpp"

#include <sys/mount.h>
#include <errno.h>

template<typename... Args>
static bool
InsMod(const char *path, Args... args)
{
  return Run("/sbin/insmod", path, args...);
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
  /* CLARA_HD requires the -f option to for reboot to work */
  if  (DetectKoboModel() == KoboModel::CLARA_HD || DetectKoboModel() == KoboModel::LIBRA2)
  {
    return Run("/sbin/reboot", "-f");
  }
  return Run("/sbin/reboot");
#else
  return false;
#endif
}

bool
KoboPowerOff()
{
#ifdef KOBO
  char buffer[256];
  if (SiblingPath("PowerOff", buffer, sizeof(buffer)))
    execl(buffer, buffer, nullptr);

  /* fall back */
  return Run("/sbin/poweroff");
#else
  return false;
#endif
}

bool
KoboUmountData()
{
#ifdef KOBO
  return umount("/mnt/onboard") == 0 || errno == EINVAL;
#else
  return true;
#endif
}

bool
KoboMountData()
{
#ifdef KOBO
  Run("/bin/dosfsck", "-a", "-w", "/dev/mmcblk0p3");
  return mount("/dev/mmcblk0p3", "/mnt/onboard", "vfat",
               MS_NOATIME|MS_NODEV|MS_NOEXEC|MS_NOSUID,
               "iocharset=utf8");
#else
  return true;
#endif
}

bool
KoboExportUSBStorage()
{
#ifdef KOBO
  bool result = false;

  RmMod("g_ether");
  RmMod("g_file_storage");

  switch (DetectKoboModel())
  {
  case KoboModel::UNKNOWN: // Let unknown try the old device
  case KoboModel::MINI:
  case KoboModel::TOUCH:
  case KoboModel::AURA:
  case KoboModel::GLO: // TODO: is this correct?
    InsMod("/drivers/ntx508/usb/gadget/arcotg_udc.ko");
    result = InsMod("/drivers/ntx508/usb/gadget/g_file_storage.ko",
                    "file=/dev/mmcblk0p3", "stall=0", "removable=1",
                    "product_id=Kobo");
    break;

  case KoboModel::TOUCH2:
  case KoboModel::GLO_HD:
  case KoboModel::AURA2:
    InsMod("/drivers/mx6sl-ntx/usb/gadget/arcotg_udc.ko");
    result = InsMod("/drivers/mx6sl-ntx/usb/gadget/g_file_storage.ko",
                    "file=/dev/mmcblk0p3", "stall=0", "removable=1",
                    "product_id=Kobo");
    break;

  case KoboModel::CLARA_HD:
  case KoboModel::LIBRA2:
    InsMod("/drivers/mx6sll-ntx/usb/gadget/configfs.ko");
    InsMod("/drivers/mx6sll-ntx/usb/gadget/libcomposite.ko");
    InsMod("/drivers/mx6sll-ntx/usb/gadget/usb_f_mass_storage.ko");
    result = InsMod("/drivers/mx6sll-ntx/usb/gadget/g_file_storage.ko",
                    "file=/dev/mmcblk0p3", "stall=0", "removable=1",
                    "product_id=Kobo");
    break;

  case KoboModel::NIA:
    InsMod("/drivers/mx6ull-ntx/usb/gadget/configfs.ko");
    InsMod("/drivers/mx6ull-ntx/usb/gadget/libcomposite.ko");
    InsMod("/drivers/mx6ull-ntx/usb/gadget/usb_f_mass_storage.ko");
    result = InsMod("/drivers/mx6ull-ntx/usb/gadget/g_file_storage.ko",
                    "file=/dev/mmcblk0p3", "stall=0", "removable=1",
                    "product_id=Kobo");
    break;
  }
  return result;
#else
  return true;
#endif
}

void
KoboUnexportUSBStorage()
{
#ifdef KOBO
  if(DetectKoboModel() == KoboModel::CLARA_HD || DetectKoboModel() == KoboModel::LIBRA2)
  {
    RmMod("g_file_storage");
    RmMod("usb_f_mass_storage");
    RmMod("libcomposite");
    RmMod("configfs");
  }
  else
  {
    RmMod("g_ether");
    RmMod("g_file_storage");
    RmMod("arcotg_udc");
  }
#endif
}

bool
IsKoboWifiOn()
{
#ifdef KOBO
  char path[64];
  sprintf(path, "/sys/class/net/%s", GetKoboWifiInterface());
  return Directory::Exists(Path{path});
#else
  return false;
#endif
}

bool
KoboWifiOn()
{
#ifdef KOBO

  switch (DetectKoboModel())
  {
  case KoboModel::UNKNOWN: // Let unknown try the old device
  case KoboModel::MINI:
  case KoboModel::TOUCH:
  case KoboModel::AURA:
  case KoboModel::GLO: // TODO: is this correct?
    InsMod("/drivers/ntx508/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/ntx508/wifi/dhd.ko");
    break;

  case KoboModel::TOUCH2:
  case KoboModel::GLO_HD:
    InsMod("/drivers/mx6sl-ntx/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/mx6sl-ntx/wifi/dhd.ko");
    break;

  case KoboModel::AURA2:
    InsMod("/drivers/mx6sl-ntx/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/mx6sl-ntx/wifi/8189fs.ko");
    break;

  case KoboModel::CLARA_HD:
    InsMod("/drivers/mx6sll-ntx/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/mx6sll-ntx/wifi/8189fs.ko");
    break;

  case KoboModel::NIA:
    InsMod("/drivers/mx6ull-ntx/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/mx6ull-ntx/wifi/8189fs.ko");
    break;

  case KoboModel::LIBRA2:
    InsMod("/drivers/mx6sll-ntx/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/mx6sll-ntx/wifi/8723ds.ko");
    break;
  }

  Sleep(2000);

  const char *interface = GetKoboWifiInterface();
  const char *driver = DetectKoboModel() == KoboModel::LIBRA2 ? "nl80211" : "wext";

  Run("/sbin/ifconfig", interface, "up");
  Run("/sbin/iwconfig", interface, "power", "off");
  Run("/bin/wlarm_le", "-i", interface, "up");
  Run("/bin/wpa_supplicant", "-i", interface,
      "-c", "/etc/wpa_supplicant/wpa_supplicant.conf",
      "-C", "/var/run/wpa_supplicant", "-B", "-D", driver);

  Sleep(2000);

  Start("/sbin/udhcpc", "-S", "-i", interface,
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
  const char *interface =  GetKoboWifiInterface();
  Run("/usr/bin/killall", "wpa_supplicant", "udhcpc");
  Run("/bin/wlarm_le", "-i", interface, "down");
  Run("/sbin/ifconfig", interface, "down");

  RmMod("dhd");
  RmMod("8189fs");
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
  File::CreateExclusive(Path("/mnt/onboard/XCSoarData/kobo/start_nickel"));

  /* unfortunately, a bug in the Kobo applications forces us to reboot
     the Kobo at this point */
  KoboReboot();
#endif
}

void
KoboRunXCSoar([[maybe_unused]] const char *mode)
{
#ifdef KOBO
  char buffer[256];
  const char *cmd = buffer;

  if (!SiblingPath("xcsoar", buffer, sizeof(buffer)))
    cmd = "/mnt/onboard/XCSoar/xcsoar";

  Run(cmd, mode);
#endif
}

void
KoboRunTelnetd()
{
#ifdef KOBO
  /* telnetd requires /dev/pts - mount it (if it isn't already) */
  if (mkdir("/dev/pts", 0777) == 0)
    mount("none", "/dev/pts", "devpts", MS_RELATIME, NULL);

  Run("/usr/sbin/telnetd", "-l", "/bin/sh");
#endif
}

void
KoboRunFtpd()
{
#ifdef KOBO
  /* ftpd needs to be fired through tcpsvd (or inetd) */
  Start("/usr/bin/tcpsvd", "-E", "0.0.0.0", "21", "ftpd", "-w", "/mnt/onboard");
#endif
}

bool
KoboCanChangeBacklightBrightness()
{
#ifdef KOBO
  switch (DetectKoboModel()) {
    case KoboModel::GLO_HD:
    case KoboModel::LIBRA2:
      return true;
    default:
      return false;
  }
#endif
  return false;
}

int
KoboGetBacklightBrightness()
{
#ifdef KOBO

  char line[4];
  int result = 0;
  switch (DetectKoboModel()) {
    case KoboModel::GLO_HD:
      if (File::ReadString(Path("/sys/class/backlight/mxc_msp430_fl.0/brightness"), line, sizeof(line))) {
        result = atoi(line);
      }
      break;
    case KoboModel::LIBRA2:
      if (File::ReadString(Path("/sys/class/backlight/mxc_msp430.0/brightness"), line, sizeof(line))) {
        result = atoi(line);
      }
      break;
    default:
      // nothing to do here...
      break;
  }
  return result;
#else
  return 0;
#endif
}

void
KoboSetBacklightBrightness([[maybe_unused]] int percent)
{
#ifdef KOBO

  if(percent < 0) { percent = 0; }
  if(percent > 100) { percent = 100; }

  switch (DetectKoboModel()) {
    case KoboModel::GLO_HD:
      File::WriteExisting(Path("/sys/class/backlight/mxc_msp430_fl.0/brightness"), std::to_string(percent).c_str());
      break;
    case KoboModel::LIBRA2:
      File::WriteExisting(Path("/sys/class/backlight/mxc_msp430.0/brightness"), std::to_string(percent).c_str());
      break;
    default:
      // nothing to do here...
      break;
  }
#endif
}
