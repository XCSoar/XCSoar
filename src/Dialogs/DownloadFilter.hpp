// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Repository/FileType.hpp"

#include <span>

struct AvailableFile;

/**
 * The filter of the download pickers, shared by all file types: the
 * same countries and the same search text apply whether a map, a
 * waypoint file or an airspace file is being picked.
 *
 * The country selection is kept in the profile
 * (ProfileKeys::DownloadAreaFilter, the area codes separated by
 * commas); the search text lives only for the session.
 */
namespace DownloadFilter {

/**
 * Does the filter concern this kind of file?  Countries make sense
 * for the files that exist per country - maps, waypoints, airspaces,
 * RASP; a firmware image or a checklist gets the plain
 * picker without filter rows.
 */
[[gnu::const]]
bool
AppliesTo(FileType type) noexcept;

/**
 * Load the country selection from the profile.  The pickers call it
 * when they open; calling it twice is harmless.
 */
void
LoadFromProfile() noexcept;

/**
 * No countries ticked - every file passes the area part?
 */
[[gnu::pure]]
bool
IsAllAreas() noexcept;

/**
 * Does this file pass the country selection?  A file without an area
 * (a global one) always does.
 */
[[gnu::pure]]
bool
MatchesArea(const AvailableFile &file) noexcept;

/**
 * Does this file pass the search text?  Case-insensitive, matched
 * against the name and the description; an empty text passes
 * everything.
 */
[[gnu::pure]]
bool
MatchesSearch(const AvailableFile &file) noexcept;

[[gnu::pure]]
const char *
GetSearchText() noexcept;

void
SetSearchText(const char *text) noexcept;

/**
 * Describe the country selection for the filter row: "All" when
 * nothing is ticked, otherwise the country names separated by
 * commas (translated).
 */
const char *
FormatAreas(std::span<char> buffer) noexcept;

/**
 * Let the user tick countries - a checkbox list like the InfoBox
 * group picker, fed with the areas the repository announces.  On OK
 * the selection goes to the profile.
 *
 * @return true if the user confirmed (the selection may still be
 * the same)
 */
bool
EditAreas() noexcept;

} // namespace DownloadFilter
