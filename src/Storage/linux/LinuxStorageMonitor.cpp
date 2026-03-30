// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxStorageMonitor.hpp"
#include "LinuxStorageDevice.hpp"
#include "io/FileLineReader.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/UnescapeCString.hpp"

#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>

struct StorageVolume {
  AllocatedPath device;
  AllocatedPath mount_point;
  std::string fs_type;
};

static void TriggerAutofsMounts();
static std::vector<StorageVolume> ParseMounts();
static bool IsRemovableBlock(const AllocatedPath &device_path);

std::vector<std::shared_ptr<StorageDevice>>
LinuxStorageMonitor::Enumerate()
{
  /* Wake any autofs-managed mount points before reading the mount
     table.  systemd .automount units only populate /proc/self/mounts
     with the real block device after the mount point is accessed. */
  TriggerAutofsMounts();

  std::vector<std::shared_ptr<StorageDevice>> result;

  for (auto &vol : ParseMounts()) {
    if (IsRemovableBlock(vol.device)) {
      result.push_back(std::make_shared<LinuxStorageDevice>(
          std::move(vol.mount_point), std::move(vol.device), vol.fs_type));
    }
  }

  return result;
}

/**
 * Scan /proc/self/mounts for autofs entries and access each mount
 * point to trigger the automounter.  This is a no-op when no autofs
 * mounts exist.
 */
static void
TriggerAutofsMounts()
try {
  FileLineReaderA reader(Path("/proc/self/mounts"));

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    const char *device = line;
    const char *sep1 = std::strchr(device, ' ');
    if (sep1 == nullptr)
      continue;

    const char *mount = sep1 + 1;
    const char *sep2 = std::strchr(mount, ' ');
    if (sep2 == nullptr)
      continue;

    const char *fstype = sep2 + 1;
    const char *sep3 = std::strchr(fstype, ' ');

    const std::string fs_str = sep3 != nullptr
      ? std::string(fstype, sep3) : std::string(fstype);

    if (fs_str != "autofs")
      continue;

    std::string mount_str(mount, sep2);
    const std::string mount_path = UnescapeCString(mount_str);

    /* Opening the directory triggers the automount daemon.
       A plain stat() is not sufficient — autofs responds to
       stat() with its own metadata without mounting.  opendir()
       forces the kernel to resolve the underlying filesystem. */
    DIR *d = ::opendir(mount_path.c_str());
    if (d != nullptr)
      ::closedir(d);
  }
} catch (...) {
}

static std::vector<StorageVolume>
ParseMounts()
try {
  std::vector<StorageVolume> result;

  FileLineReaderA reader(Path("/proc/self/mounts"));

  const char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    /* /proc/mounts format: device mountpoint fstype options dump pass */
    const char *device = line;
    const char *sep1 = std::strchr(device, ' ');
    if (sep1 == nullptr)
      continue;

    const char *mount = sep1 + 1;
    const char *sep2 = std::strchr(mount, ' ');
    if (sep2 == nullptr)
      continue;

    const char *fstype = sep2 + 1;
    const char *sep3 = std::strchr(fstype, ' ');

    std::string device_str(device, sep1);

    // Only block devices
    if (device_str.rfind("/dev/", 0) != 0)
      continue;

    StorageVolume vol;
    vol.device = AllocatedPath(device_str.c_str());
    // /proc/self/mounts may contain escape sequences for special chars
    std::string mount_str(mount, sep2);
    vol.mount_point = AllocatedPath(UnescapeCString(mount_str).c_str());
    vol.fs_type = sep3 != nullptr
      ? std::string(fstype, sep3) : std::string(fstype);

    result.push_back(std::move(vol));
  }

  return result;
} catch (...) {
  return {};
}

static bool
IsRemovableBlock(const AllocatedPath &device_path)
{
  const std::string dev_utf8 = device_path.ToUTF8();
  if (dev_utf8.rfind("/dev/", 0) != 0)
    return false;

  // Resolve the base (whole-disk) device name for a partition so that
  // sysfs attributes like "removable" are read from the right entry.
  std::string name = dev_utf8.substr(5);
  const std::string sysfs_class = "/sys/class/block/" + name;
  {
    const AllocatedPath partition_path =
      AllocatedPath((sysfs_class + "/partition").c_str());
    if (File::Exists(partition_path)) {
      std::string target;
      if (File::ReadLink(AllocatedPath(sysfs_class.c_str()), target)) {
        const AllocatedPath parent = Path(target.c_str()).GetParent();
        const Path base = Path(parent).GetBase();
        if (base != nullptr) {
          const std::string base_name = base.ToUTF8();
          // Verify the resolved name is a real block device in sysfs;
          // some kernel versions use a flat symlink layout that makes
          // the parent path point to a non-device directory.
          if (!base_name.empty() &&
              File::Exists(AllocatedPath(
                ("/sys/class/block/" + base_name).c_str())))
            name = base_name;
        }
      }
    }
  }

  /* If the name still looks like a partition (no "removable" sysfs
     attribute), derive the base device by stripping the numeric
     partition suffix.  This handles kernels where the sysfs symlink
     layout changed. */
  {
    const std::string removable_path =
      "/sys/class/block/" + name + "/removable";
    if (!File::Exists(AllocatedPath(removable_path.c_str()))) {
      std::string stripped = dev_utf8.substr(5);
      while (!stripped.empty() &&
             std::isdigit(static_cast<unsigned char>(stripped.back())))
        stripped.pop_back();
      if (!stripped.empty() && stripped.back() == 'p')
        stripped.pop_back();
      if (!stripped.empty() && stripped != dev_utf8.substr(5))
        name = stripped;
    }
  }

  const std::string sysfs_base = "/sys/class/block/" + name;

  {
    const AllocatedPath p = AllocatedPath((sysfs_base + "/removable").c_str());
    char buf[16];
    if (File::ReadString(p, buf, sizeof(buf))) {
      if (atoi(buf) == 1)
        return true;
    }
  }

  {
    const AllocatedPath subsystem_link =
      AllocatedPath((sysfs_base + "/device/subsystem").c_str());
    std::string target;
    if (File::ReadLink(subsystem_link, target)) {
      const Path target_path(target.c_str());
      const Path target_base = target_path.GetBase();
      if (target_base.c_str()) {
        const std::string subsystem = target_base.ToUTF8();
        if (subsystem.find("mmc") != std::string::npos)
          return true;
        if (subsystem.find("usb") != std::string::npos)
          return true;
        // SATA (ata) but not PATA (legacy IDE)
        if (subsystem.find("ata") != std::string::npos &&
            subsystem.find("pata") == std::string::npos)
          return true;
      }
    }
  }

  {
    const AllocatedPath p = AllocatedPath((sysfs_base + "/device/uevent").c_str());
    char buf[4096];
    if (File::ReadString(p, buf, sizeof(buf))) {
      const std::string uevent(buf);
      if (uevent.find("DEVTYPE=usb") != std::string::npos)
        return true;
      if (uevent.find("MMC_TYPE=SD") != std::string::npos)
        return true;
      if (uevent.find("MMC_TYPE=MMC") != std::string::npos)
        return true;
      if (uevent.find("MMC_TYPE=SDIO") != std::string::npos)
        return true;
    }
  }

  return false;
}
