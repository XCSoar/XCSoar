// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <string_view>

struct Waypoint;

/**
 * Maximum length (including the terminating NUL) of a normalised
 * waypoint name we will operate on.  Inputs longer than this are
 * silently rejected by the search helpers; in practice waypoint
 * names are far shorter.  Used to size stack buffers in the
 * substring-search code paths.
 */
inline constexpr std::size_t NAME_SEARCH_BUFFER_SIZE = 4096;

/**
 * Test whether the already-normalised @p needle (uppercase
 * alphanumeric, NUL-terminated, as produced by
 * NormalizeSearchString()) appears as a substring of either the
 * normalised long name or the normalised shortname of @p wp.
 *
 * An empty @p needle matches every waypoint.
 *
 * Names longer than NAME_SEARCH_BUFFER_SIZE are not matched.
 */
[[gnu::pure]]
bool
WaypointMatchesNormalisedSubstring(const Waypoint &wp,
                                   const char *needle) noexcept;
