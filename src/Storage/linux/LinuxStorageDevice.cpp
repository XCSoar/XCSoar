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
#include <memory>

namespace {
struct DirCloser {
  void operator()(DIR *d) const noexcept { if (d) closedir(d); }
};
using UniqueDir = std::unique_ptr<DIR, DirCloser>;
} // namespace

LinuxStorageDevice::LinuxStorageDevice(AllocatedPath mount_point,
                                       AllocatedPath device,
                                       std::string fs_type) noexcept
  : mount_point_(std::move(mount_point)), device_(std::move(device)),
    fs_type_(std::move(fs_type)) {}

std::string
LinuxStorageDevice::Id() const {
  return device_.ToUTF8();
}

std::string
LinuxStorageDevice::Name() const {
  return mount_point_.ToUTF8();
}

std::string
LinuxStorageDevice::Label() const {
  // Safely obtain the device base name; fall back to mount name if absent.
  const Path dev_path(device_.c_str());
  const Path dev_base = dev_path.GetBase();
  std::string dev_basename;
  if (dev_base.c_str())
    dev_basename = dev_base.ToUTF8();
  else
    dev_basename = Name();

  UniqueDir d(opendir("/dev/disk/by-label"));
  if (d) {
    struct dirent *ent;
    while ((ent = readdir(d.get())) != nullptr) {
      if (ent->d_name[0] == '.')
        continue;
      const std::string entrypath = std::string("/dev/disk/by-label/") + ent->d_name;
      std::string target;
      if (!File::ReadLink(Path(entrypath.c_str()), target))
        continue;
      const Path target_path(target.c_str());
      const Path target_base = target_path.GetBase();
      if (!target_base.c_str())
        continue; // skip entries with no base name
      const std::string target_basename = target_base.ToUTF8();
      if (target_basename == dev_basename)
        return UnescapeCString(std::string(ent->d_name));
    }
  }

  // fallback: use mount point
  return Name();
}

StorageDevice::Kind
LinuxStorageDevice::GetKind() const {
  return Kind::Removable;
}

bool
LinuxStorageDevice::IsWritable() const
{
  return Directory::IsWritable(mount_point_);
}

std::unique_ptr<Reader>
LinuxStorageDevice::OpenRead(Path path) const
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  return std::make_unique<FileReader>(p);
}

std::unique_ptr<OutputStream>
LinuxStorageDevice::OpenWrite(Path path, bool truncate)
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  auto mode = FileOutputStream::Mode::CREATE;
  if (!truncate)
    mode = FileOutputStream::Mode::APPEND_OR_CREATE;
  return std::make_unique<FileOutputStream>(p, mode);
}

std::optional<StorageDevice::Space>
LinuxStorageDevice::GetSpace() const noexcept
{
  struct statvfs st;
  const char *mp = mount_point_.c_str();
  if (statvfs(mp, &st) != 0)
    return std::nullopt;

  uint64_t total = uint64_t(st.f_blocks) * uint64_t(st.f_frsize);
  uint64_t free  = uint64_t(st.f_bavail) * uint64_t(st.f_frsize);
  return StorageDevice::Space{total, free};
}
