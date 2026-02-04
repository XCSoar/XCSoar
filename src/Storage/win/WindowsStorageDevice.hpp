// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageDevice.hpp"
#include "system/Path.hpp"

#include <memory>
#include <string>

class WindowsStorageDevice : public StorageDevice {
  const unsigned drive_type_;
  AllocatedPath mount_point_;

public:
  WindowsStorageDevice(AllocatedPath mount_point,
                       unsigned drive_type) noexcept;

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

private:
  Kind MapDriveType() const noexcept;
};
