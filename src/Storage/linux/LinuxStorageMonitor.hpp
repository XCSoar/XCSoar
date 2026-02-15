// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "Storage/StorageMonitor.hpp"

#include <memory>
#include <string>
#include <vector>

class LinuxStorageMonitor : public StorageMonitor {
public:
  std::vector<std::shared_ptr<StorageDevice>> Enumerate() override;
};
