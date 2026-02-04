// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StorageMonitor.hpp"

#include <memory>

/**
 * Factory for the platform-specific storage monitor implementation.
 */
std::unique_ptr<StorageMonitor> CreatePlatformStorageMonitor();
