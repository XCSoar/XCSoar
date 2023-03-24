// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <optional>

class ZipArchive;

/**
 * Obtain the configured map file path from the profile and open it as
 * a ZIP file.
 *
 * Throws on error.
 *
 * @return std::nullopt if no mpa file is configured
 */
std::optional<ZipArchive>
OpenMapFile();
