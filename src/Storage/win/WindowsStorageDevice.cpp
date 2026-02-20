// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowsStorageDevice.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"

#include <string>
#include <windows.h>

WindowsStorageDevice::WindowsStorageDevice(AllocatedPath mount_point,
                                           unsigned drive_type) noexcept
  : drive_type_(drive_type),
    mount_point_(std::move(mount_point)) {}

std::string
WindowsStorageDevice::Id() const
{
  return mount_point_.ToUTF8();
}

std::string
WindowsStorageDevice::Name() const
{
  return mount_point_.ToUTF8();
}

std::string
WindowsStorageDevice::Label() const
{
  char volname[MAX_PATH];
  if (GetVolumeInformationA(mount_point_.c_str(), volname, MAX_PATH, nullptr,
                            nullptr, nullptr, nullptr, 0))
    return volname;

  return Name();
}

StorageDevice::Kind
WindowsStorageDevice::GetKind() const
{
  return MapDriveType();
}

StorageDevice::Kind
WindowsStorageDevice::MapDriveType() const noexcept
{
  switch (drive_type_) {
  case DRIVE_FIXED:
    return Kind::Internal;
  case DRIVE_REMOVABLE:
  default:
    return Kind::Removable;
  }
}

bool
WindowsStorageDevice::IsWritable() const
{
  return Directory::IsWritable(mount_point_);
}

std::unique_ptr<Reader>
WindowsStorageDevice::OpenRead(Path path) const
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  return std::make_unique<FileReader>(p);
}

std::unique_ptr<OutputStream>
WindowsStorageDevice::OpenWrite(Path path, bool truncate)
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  auto mode = FileOutputStream::Mode::CREATE;

  if (!truncate)
    mode = FileOutputStream::Mode::APPEND_OR_CREATE;

  return std::make_unique<FileOutputStream>(p, mode);
}

std::optional<StorageDevice::Space>
WindowsStorageDevice::GetSpace() const noexcept
{
  ULARGE_INTEGER freeAvail, totalBytes, freeBytes;
  if (!GetDiskFreeSpaceExA(mount_point_.c_str(), &freeAvail, &totalBytes,
                           &freeBytes))
    return std::nullopt;

  return StorageDevice::Space{static_cast<uint64_t>(totalBytes.QuadPart),
                              static_cast<uint64_t>(freeAvail.QuadPart)};
}
