// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageDevice.hpp"
#include "system/Path.hpp"

#include <memory>
#include <string>

class LinuxStorageDevice : public StorageDevice {
  AllocatedPath mount_point_;
  AllocatedPath device_;
  std::string fs_type_;

public:
  LinuxStorageDevice(AllocatedPath mount_point,
                     AllocatedPath device,
                     std::string fs_type) noexcept;

  // StorageDevice overrides
  std::string Id() const override;
  std::string Name() const override;
  std::string Label() const override;
  Kind GetKind() const override;

  bool IsWritable() const override;

  std::unique_ptr<Reader>
  OpenRead(Path path) const override;

  std::unique_ptr<OutputStream>
  OpenWrite(Path path, bool truncate = true) override;

  std::optional<Space> GetSpace() const noexcept override;
};
