// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowsStorageDevice.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "util/ConvertString.hpp"

#include <string>
#include <cstdio>
#include <windows.h>

WindowsStorageDevice::WindowsStorageDevice(AllocatedPath mount_point) noexcept
  : mount_point_(std::move(mount_point)) {}

std::string
WindowsStorageDevice::id() const
{
  return mount_point_.ToUTF8();
}

std::string
WindowsStorageDevice::name() const
{
  return mount_point_.ToUTF8();
}

std::string
WindowsStorageDevice::label() const
{
  wchar_t volname[MAX_PATH];
  auto mount_wide = UTF8ToWide(mount_point_.ToUTF8());
  if (GetVolumeInformationW(mount_wide.c_str(), volname, MAX_PATH, nullptr,
                            nullptr, nullptr, nullptr, 0))
    return WideToUTF8(volname);

  return name();
}

StorageDevice::Kind
WindowsStorageDevice::kind() const
{
  return Kind::Removable;
}

bool
WindowsStorageDevice::isWritable() const
{
  return Directory::IsWritable(mount_point_);
}

std::unique_ptr<Reader>
WindowsStorageDevice::openRead(Path path) const
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  return std::make_unique<FileReader>(p);
}

std::unique_ptr<OutputStream>
WindowsStorageDevice::openWrite(Path path, bool truncate)
{
  const AllocatedPath p = AllocatedPath::Build(mount_point_, path);
  auto mode = FileOutputStream::Mode::CREATE;

  if (!truncate)
    mode = FileOutputStream::Mode::APPEND_OR_CREATE;

  return std::make_unique<FileOutputStream>(p, mode);
}

std::optional<StorageDevice::Space>
WindowsStorageDevice::space() const noexcept
{
  ULARGE_INTEGER freeAvail, totalBytes, freeBytes;
  if (!GetDiskFreeSpaceExA(mount_point_.c_str(), &freeAvail, &totalBytes,
                           &freeBytes))
    return std::nullopt;

  return StorageDevice::Space{static_cast<uint64_t>(totalBytes.QuadPart),
                              static_cast<uint64_t>(freeAvail.QuadPart)};
}
