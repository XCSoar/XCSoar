// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/MOSMIX/AutoUpdate.hpp"
#include "Profile/Current.hpp"
#include "Profile/Map.hpp"
#include "Profile/Keys.hpp"
#include "time/BrokenDate.hpp"
#include "TestUtil.hpp"

static constexpr BrokenDate TODAY{2026, 9, 20};
static constexpr BrokenDate TOMORROW{2026, 9, 21};

static void
Forget() noexcept
{
  Profile::map.Clear();
}

int
main()
{
  plan_tests(15);

  /* nothing known yet: fetch, and there is nothing to show */
  Forget();
  ok1(MOSMIX::ShouldFetchToday(TODAY));
  ok1(!MOSMIX::GetStoredForecast(TODAY).has_value());

  /* an implausible date is no date at all */
  ok1(!MOSMIX::ShouldFetchToday(BrokenDate::Invalid()));
  ok1(!MOSMIX::GetStoredForecast(BrokenDate::Invalid()).has_value());

  /* once fetched, the answer is kept and the network is left alone */
  Forget();
  MOSMIX::RememberFetch(TODAY, Temperature::FromKelvin(300));
  ok1(!MOSMIX::ShouldFetchToday(TODAY));
  ok1(MOSMIX::GetStoredForecast(TODAY).has_value());
  ok1(equals(MOSMIX::GetStoredForecast(TODAY)->ToKelvin(), 300));

  /* a fetch that came back empty is still an answer for the day */
  Forget();
  MOSMIX::RememberFetch(TODAY, std::nullopt);
  ok1(!MOSMIX::ShouldFetchToday(TODAY));
  ok1(!MOSMIX::GetStoredForecast(TODAY).has_value());

  /* the pilot's own number wins for the rest of the day -- both
     against a new fetch and against the one already stored, which is
     the way round that used to be missed */
  Forget();
  MOSMIX::RememberFetch(TODAY, Temperature::FromKelvin(300));
  MOSMIX::RememberManualEntry(TODAY);
  ok1(!MOSMIX::ShouldFetchToday(TODAY));
  ok1(!MOSMIX::GetStoredForecast(TODAY).has_value());

  /* and only for that day */
  ok1(MOSMIX::ShouldFetchToday(TOMORROW));
  ok1(!MOSMIX::GetStoredForecast(TOMORROW).has_value());

  /* yesterday's number is not today's */
  Forget();
  MOSMIX::RememberFetch(TODAY, Temperature::FromKelvin(300));
  ok1(MOSMIX::ShouldFetchToday(TOMORROW));
  ok1(!MOSMIX::GetStoredForecast(TOMORROW).has_value());

  return exit_status();
}
