// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageDevice.hpp"
#include "system/Path.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"

#include <memory>
#include <string>

class LinuxStorageDevice : public StorageDevice {
public:
  LinuxStorageDevice(AllocatedPath mount_point,
                     AllocatedPath device,
                     std::string fs_type) noexcept;

  // StorageDevice overrides
  std::string id() const override;
  std::string name() const override;
  std::string label() const override;
  Kind kind() const override;

  bool isWritable() const override;

  std::unique_ptr<Reader>
  openRead(Path path) const override;

  std::unique_ptr<OutputStream>
  openWrite(Path path, bool truncate = true) override;

  std::optional<Space> space() const noexcept override;

private:
  AllocatedPath mount_point_;
  AllocatedPath device_;
  std::string fs_type_;
};
