// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

class MultiFileDataField;

/**
 * Build an export directory path from a base target and subfolder.
 */
AllocatedPath
BuildExportDirectory(Path target, std::string_view subfolder) noexcept;

/**
 * Scan for files matching any of the patterns and populate the data field.
 */
void
ScanFilesIntoDataField(const AllocatedPath &path, MultiFileDataField &df,
                       std::initializer_list<std::string_view> patterns,
                       bool recursive = true) noexcept;

/**
 * Convert a string to UTF-8.
 */
std::string
ToUtf8String(const char *text) noexcept;

/**
 * Map common errno values to localized user messages (UTF-8).
 */
std::string
MapErrnoToMessage(int err) noexcept;
