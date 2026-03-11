// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"

namespace EDL {

static constexpr unsigned NUM_ISOBARS = 5;
static constexpr unsigned ISOBARS[NUM_ISOBARS] = {
  50000,
  60000,
  70000,
  80000,
  90000,
};

[[gnu::pure]]
bool
IsSupportedIsobar(unsigned isobar) noexcept;

void
FormatForecastParameter(char *buffer, const BrokenDateTime &forecast) noexcept;

[[gnu::const]]
StaticString<256>
BuildDownloadUrl(const BrokenDateTime &forecast, unsigned isobar) noexcept;

[[gnu::const]]
StaticString<64>
BuildCacheFileName(const BrokenDateTime &forecast, unsigned isobar) noexcept;

AllocatedPath
BuildCachePath(const BrokenDateTime &forecast, unsigned isobar) noexcept;

} // namespace EDL
