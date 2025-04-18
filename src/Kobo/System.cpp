// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  case KoboModel::CLARA_2E:
  case KoboModel::LIBRA2:
  case KoboModel::LIBRA_H2O:
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
  KoboModel kobo_model = DetectKoboModel();
  if(kobo_model == KoboModel::CLARA_HD || kobo_model == KoboModel::CLARA_2E
      || kobo_model == KoboModel::LIBRA2 || kobo_model == KoboModel::LIBRA_H2O)
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
  case KoboModel::LIBRA_H2O:
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

  case KoboModel::CLARA_2E:
    InsMod("/drivers/mx6sll-ntx/wifi/sdio_wifi_pwr.ko");
    InsMod("/drivers/mx6sll-ntx/wifi/mlan.ko");
    InsMod("/drivers/mx6sll-ntx/wifi/moal.ko", "mod_para=nxp/wifi_mod_para_sd8987.conf");
    break;
  }

  Sleep(2000);

  const char *interface = GetKoboWifiInterface();
  const char *driver = (DetectKoboModel() == KoboModel::LIBRA2
    || DetectKoboModel() == KoboModel::CLARA_2E) ? "nl80211" : "wext";

  Run("/sbin/ifconfig", interface, "up");
  Run("/sbin/iwconfig", interface, "power", "off");
  if (DetectKoboModel() != KoboModel::CLARA_2E)
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
  if (DetectKoboModel() != KoboModel::CLARA_2E)
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
  case KoboModel::CLARA_2E:
  case KoboModel::CLARA_HD:
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
  case KoboModel::CLARA_2E:
  case KoboModel::CLARA_HD:
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
  case KoboModel::CLARA_2E:
  case KoboModel::CLARA_HD:
    File::WriteExisting(Path("/sys/class/backlight/mxc_msp430.0/brightness"), std::to_string(percent).c_str());
    break;

  default:
    // nothing to do here...
    break;
  }
#endif
}

/**
 * Gets the file that contains the background colour
 * as text of integer 0-10
 *
 * Some models have more than one possibility and each
 * possibility must be checked in a certain order
 *
 * @return the file that contains the background colour
 * code or nullptr if background colour not supported
 */
const char *
KoboGetBacklightColourFile() noexcept
{
#ifdef KOBO
  constexpr const char * colour_files[3] = {
    "/sys/class/leds/aw99703-bl_FL1/color",
    "/sys/class/backlight/lm3630a_led/color",
    "/sys/class/backlight/tlc5947_bl/color"
  };
  bool files_to_check[3] = {
    false,
    false,
    false
  };

  switch (DetectKoboModel()) {
  case KoboModel::CLARA_2E:
    files_to_check[0] = true;
    break;

  case KoboModel::CLARA_HD:
    files_to_check[1] = true;
    break;

  case KoboModel::LIBRA2:
    files_to_check[0] = true;
    files_to_check[1] = true;
    files_to_check[2] = true;
    break;

  default:
    return nullptr;
  }
  if (files_to_check[0] && File::Exists(Path(colour_files[0])))
    return colour_files[0]; 
  if (files_to_check[1] && File::Exists(Path(colour_files[1])))
    return colour_files[1]; 
  if (files_to_check[2] && File::Exists(Path(colour_files[2])))
    return colour_files[2]; 
#endif
  return nullptr;
}

bool
KoboCanChangeBacklightColour() noexcept
{
#ifdef KOBO
  return KoboGetBacklightColourFile() != nullptr;
#endif
  return false;
}

/**
 * Returns true if successful in fetching the current colour
 * or false if not
 *
 * @param (by ref) colour is set to the current colour
 * where 0 = cold (yellow) and 10 is hot (white)
 */
bool
KoboGetBacklightColour([[maybe_unused]] unsigned int &colour) noexcept
{
#ifdef KOBO
  char line[4];
  if (File::ReadString(Path(KoboGetBacklightColourFile()),
      line,sizeof(line))) {
    int raw_value = atoi(line);
    if (raw_value < 0)
      colour = 0;
    else
      colour = raw_value;
    return true;
  }
#endif
  return false;
}

/**
 * Changes the colour of the background lighting
 *
 * @param colour desired colour where 0 = cold (yellow)
 * and 10 is hot (white). Other values are silently ignored
 */
void
KoboSetBacklightColour([[maybe_unused]] int colour) noexcept
{
#ifdef KOBO

  if(colour < 0) { colour = 0; }
  if(colour > 10) { colour = 10; }
  File::WriteExisting(Path(KoboGetBacklightColourFile()),
                      std::to_string(colour).c_str());
#endif
}
