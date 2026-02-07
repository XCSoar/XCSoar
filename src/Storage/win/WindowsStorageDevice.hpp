// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageDevice.hpp"
#include "system/Path.hpp"

#include <memory>
#include <string>

class WindowsStorageDevice : public StorageDevice {
public:
  explicit WindowsStorageDevice(AllocatedPath mount_point) noexcept;

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
};
