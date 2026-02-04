// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxStorageDevice.hpp"

#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/UnescapeCString.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"

#include <dirent.h>
#include <sys/statvfs.h>
#include <limits.h>

LinuxStorageDevice::LinuxStorageDevice(AllocatedPath mount_point,
                                       AllocatedPath device,
                                       std::string fs_type) noexcept
  : mount_point_(std::move(mount_point)), device_(std::move(device)),
    fs_type_(std::move(fs_type)) {}

std::string
LinuxStorageDevice::id() const {
  return device_.ToUTF8();
}

std::string
LinuxStorageDevice::name() const {
  return mount_point_.ToUTF8();
}

std::string
LinuxStorageDevice::label() const {
  const std::string dev_basename = Path(device_.c_str()).GetBase().ToUTF8();

  DIR *d = opendir("/dev/disk/by-label");
  if (d != nullptr) {
    struct dirent *ent;
    while ((ent = readdir(d)) != nullptr) {
      if (ent->d_name[0] == '.')
        continue;
      const std::string entrypath = std::string("/dev/disk/by-label/") + ent->d_name;
      std::string target;
      if (!File::ReadLink(Path(entrypath.c_str()), target))
        continue;
      const std::string target_basename = Path(target.c_str()).GetBase().ToUTF8();
      if (target_basename == dev_basename) {
        const std::string raw(ent->d_name);
        closedir(d);
        return UnescapeCString(raw);
      }
    }
    closedir(d);
  }

  // fallback: use mount point
  return name();
}

StorageDevice::Kind
LinuxStorageDevice::kind() const {
  return Kind::Removable;
}

bool
LinuxStorageDevice::isWritable() const
{
  return Directory::IsWritable(mount_point_);
}

std::unique_ptr<Reader>
LinuxStorageDevice::openRead(Path path) const
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  return std::make_unique<FileReader>(p);
}

std::unique_ptr<OutputStream>
LinuxStorageDevice::openWrite(Path path, bool truncate)
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  auto mode = FileOutputStream::Mode::CREATE;
  if (!truncate)
    mode = FileOutputStream::Mode::APPEND_OR_CREATE;
  return std::make_unique<FileOutputStream>(p, mode);
}

std::optional<StorageDevice::Space>
LinuxStorageDevice::space() const noexcept
{
  struct statvfs st;
  const char *mp = mount_point_.c_str();
  if (statvfs(mp, &st) != 0)
    return std::nullopt;

  uint64_t total = uint64_t(st.f_blocks) * uint64_t(st.f_frsize);
  uint64_t free  = uint64_t(st.f_bavail) * uint64_t(st.f_frsize);
  return StorageDevice::Space{total, free};
}
