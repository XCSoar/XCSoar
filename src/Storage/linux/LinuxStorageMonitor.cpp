// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxStorageMonitor.hpp"
#include "LinuxStorageDevice.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/UnescapeCString.hpp"

#include <fstream>
#include <sstream>
#include <cctype>
#include <string>

struct StorageVolume {
  AllocatedPath device;
  AllocatedPath mount_point;
  std::string fs_type;
};

static std::vector<StorageVolume> ParseMounts();
static bool IsRemovableBlock(const AllocatedPath &device_path);

std::vector<std::shared_ptr<StorageDevice>>
LinuxStorageMonitor::Enumerate()
{
  std::vector<std::shared_ptr<StorageDevice>> result;

  for (auto &vol : ParseMounts()) {
    if (IsRemovableBlock(vol.device)) {
      result.push_back(std::make_shared<LinuxStorageDevice>(
          std::move(vol.mount_point), std::move(vol.device), vol.fs_type));
    }
  }

  return result;
}

static std::vector<StorageVolume>
ParseMounts()
{
  std::ifstream mounts("/proc/self/mounts");
  std::vector<StorageVolume> result;

  if (!mounts.is_open())
    return result;

  std::string line;
  while (std::getline(mounts, line)) {
    std::istringstream iss(line);

    std::string device_str;
    std::string mount_str;
    std::string fs_type;
    std::string options;
    int dump, pass;

    if (!(iss >> device_str >> mount_str >> fs_type >> options >> dump >> pass))
      continue;

    // Only block devices
    if (device_str.rfind("/dev/", 0) != 0)
      continue;

    StorageVolume vol;
    vol.device = AllocatedPath(device_str.c_str());
    // /proc/self/mounts may contain escape sequences for special chars
    std::string decoded = UnescapeCString(mount_str);
    vol.mount_point = AllocatedPath(decoded.c_str());
    vol.fs_type = fs_type;

    result.push_back(std::move(vol));
  }

  return result;
}

static bool
IsRemovableBlock(const AllocatedPath &device_path)
{
  const std::string dev_utf8 = device_path.ToUTF8();
  if (dev_utf8.rfind("/dev/", 0) != 0)
    return false;

  // Prefer sysfs partition metadata to resolve the base device name.
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
          if (!base_name.empty())
            name = base_name;
        }
      }
    }
  }

  if (name == dev_utf8.substr(5)) {
    const std::string sysfs_base = "/sys/class/block/" + name;
    if (!File::Exists(AllocatedPath(sysfs_base.c_str()))) {
      // Fallback: strip partition suffix (digits and optional 'p').
      // Assumes common Linux naming: sdXN, mmcblkNpM, nvmeXnYpZ.
      while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back())))
        name.pop_back();
      if (!name.empty() && name.back() == 'p')
        name.pop_back();
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
