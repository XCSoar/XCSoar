// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AvailableFile.hpp"

#include <string>
#include <string_view>
#include <vector>

/**
 * ASCII-uppercase the repository area code.  Empty means Regions
 * (no country, including multi-country packs).
 */
[[gnu::pure]]
std::string
NormalizeFileArea(std::string_view area) noexcept;

/**
 * ISO alpha-2 from the first `_`/`-` token of the filename.
 * Empty if the token is not a country (Regions).
 */
[[gnu::pure]]
std::string
GuessFileAreaFromName(std::string_view name) noexcept;

/**
 * Repository `area=` if set, otherwise #GuessFileAreaFromName.
 */
[[gnu::pure]]
std::string
NormalizeFileArea(const AvailableFile &file) noexcept;

[[gnu::pure]]
bool
FileMatchesArea(const AvailableFile &file, std::string_view area) noexcept;

/**
 * Unique normalized area codes: Regions (empty) first, then A–Z.
 */
[[gnu::pure]]
std::vector<std::string>
CollectUniqueFileAreas(const std::vector<AvailableFile> &files);

[[gnu::pure]]
unsigned
CountFilesInArea(const std::vector<AvailableFile> &files,
                 std::string_view area) noexcept;

void
AppendFilesInArea(const std::vector<AvailableFile> &files,
                  std::string_view area,
                  std::vector<AvailableFile> &dest);
