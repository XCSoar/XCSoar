// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AutoUpdate.hpp"
#include "Profile/Current.hpp"
#include "Profile/Map.hpp"
#include "Profile/Keys.hpp"
#include "time/BrokenDate.hpp"

namespace {

/**
 * The day as one number, e.g. 20260920, which is what goes into the
 * profile: it compares and prints as a date without needing a format
 * of its own.
 */
[[gnu::const]]
int
ToDayNumber(const BrokenDate &date) noexcept
{
  return int(date.year) * 10000 + int(date.month) * 100 + int(date.day);
}

[[gnu::pure]]
bool
IsToday(std::string_view key, int today) noexcept
{
  int stored;
  return Profile::map.Get(key, stored) && stored == today;
}

} // anonymous namespace

bool
MOSMIX::ShouldFetchToday(const BrokenDate &today) noexcept
{
  if (!today.IsPlausible())
    /* without a date there is no telling what "today" means, and the
       forecast is picked by day */
    return false;

  const int day = ToDayNumber(today);

  return !IsToday(ProfileKeys::MosmixManualEntry, day) &&
    !IsToday(ProfileKeys::MosmixLastFetch, day);
}

void
MOSMIX::RememberFetch(const BrokenDate &today) noexcept
{
  if (today.IsPlausible())
    Profile::map.Set(ProfileKeys::MosmixLastFetch, ToDayNumber(today));
}

void
MOSMIX::RememberManualEntry(const BrokenDate &today) noexcept
{
  if (today.IsPlausible())
    Profile::map.Set(ProfileKeys::MosmixManualEntry, ToDayNumber(today));
}
