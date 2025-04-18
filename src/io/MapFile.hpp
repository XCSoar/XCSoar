// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <optional>

class ZipArchive;
class ZipReader;

/**
 * Obtain the configured map file path from the profile and open it as
 * a ZIP file.
 *
 * Throws on error.
 *
 * @return std::nullopt if no map file is configured
 */
std::optional<ZipArchive>
OpenMapFile();

/**
 * Open a file inside the configured map file.
 *
 * Throws on error.
 *
 * @return std::nullopt if no map file is configured
 */
std::optional<ZipReader>
OpenInMapFile(const char *filename);
