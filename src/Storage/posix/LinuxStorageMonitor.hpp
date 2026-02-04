// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "Storage/StorageMonitor.hpp"

#include <memory>
#include <string>
#include <vector>

struct StorageVolume {
  AllocatedPath device;
  AllocatedPath mount_point;
  std::string fs_type;
};

std::vector<StorageVolume> parseMounts();

bool isRemovableBlock(const AllocatedPath &device_path);

class LinuxStorageMonitor : public StorageMonitor {
public:
  std::vector<std::shared_ptr<StorageDevice>> enumerate() override;
};
