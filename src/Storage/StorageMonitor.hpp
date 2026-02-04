// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StorageDevice.hpp"

#include <memory>
#include <vector>

class StorageMonitor {
public:
  virtual ~StorageMonitor() = default;

  /* Enumerate available storage devices */
  virtual std::vector<std::shared_ptr<StorageDevice>>
    Enumerate() = 0;
};
