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

std::optional<Temperature>
MOSMIX::GetStoredForecast(const BrokenDate &today) noexcept
{
  if (!today.IsPlausible())
    return std::nullopt;

  const int day = ToDayNumber(today);

  /* the pilot's own number wins for the rest of the day.  This is
     checked here and not only in ShouldFetchToday(), because the
     caller asks this question first: after a fetch earlier today the
     stored forecast would otherwise be handed back and put into the
     field, over the value that was typed in. */
  if (IsToday(ProfileKeys::MosmixManualEntry, day) ||
      !IsToday(ProfileKeys::MosmixLastFetch, day))
    return std::nullopt;

  double kelvin;
  if (!Profile::map.Get(ProfileKeys::MosmixForecast, kelvin) || kelvin <= 0)
    /* fetched today, but it had nothing to say */
    return std::nullopt;

  return Temperature::FromKelvin(kelvin);
}

void
MOSMIX::RememberFetch(const BrokenDate &today,
                      std::optional<Temperature> value) noexcept
{
  if (!today.IsPlausible())
    return;

  Profile::map.Set(ProfileKeys::MosmixLastFetch, ToDayNumber(today));

  /* zero rather than absent, so that yesterday's number cannot be
     mistaken for today's silence */
  Profile::map.Set(ProfileKeys::MosmixForecast,
                   value.has_value() ? value->ToKelvin() : 0.0);
}

void
MOSMIX::RememberManualEntry(const BrokenDate &today) noexcept
{
  if (today.IsPlausible())
    Profile::map.Set(ProfileKeys::MosmixManualEntry, ToDayNumber(today));
}
