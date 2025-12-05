// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <optional>

class ZipArchive;
class ZipReader;
class Path;

/**
 * Open a waypoint data archive (.xcd) file as a ZIP archive.
 *
 * Throws on error.
 *
 * @return std::nullopt if path is invalid
 */
std::optional<ZipArchive>
OpenWaypointDataFile(Path path);

/**
 * Open a file inside a waypoint data archive.
 *
 * Throws on error.
 *
 * @return std::nullopt if archive cannot be opened or file not found
 */
std::optional<ZipReader>
OpenInWaypointDataFile(Path archive_path, const char *filename);
