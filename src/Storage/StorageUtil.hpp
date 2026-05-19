// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <memory>
#include <string>

class StorageDevice;

/**
 * Check whether a path is a content:// URI (Android SAF).
 */
[[nodiscard]]
bool IsContentUri(Path path) noexcept;

/**
 * Format a storage location path for user-visible display.
 *
 * For filesystem paths returns the path as-is.
 * For content:// URIs extracts a human-readable volume label.
 */
[[nodiscard]]
std::string FormatStorageCaption(Path path);

/**
 * Extract the subfolder path from a SAF content:// URI.
 *
 * E.g. "content://…/tree/primary%3Axcsoar-test" → "xcsoar-test".
 * Returns an empty string when the URI points at the volume root.
 */
[[nodiscard]]
std::string ExtractSafSubfolder(const std::string &uri);

/**
 * Check whether a device identifier starts with "saf:".
 */
[[nodiscard]]
bool IsSafDeviceId(const std::string &id) noexcept;

/**
 * Find a writable StorageDevice whose Name() matches the given
 * path.  Returns nullptr if no match is found or no monitor is
 * available.
 */
[[nodiscard]]
std::shared_ptr<StorageDevice>
FindDeviceByName(Path name) noexcept;
