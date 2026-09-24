// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "time/PosixTimeZone.hpp"
#include "time/TimeZones.hpp"
#include "TestUtil.hpp"

using namespace std::chrono;

/**
 * Compose a UTC time point from its broken-down representation.
 */
static constexpr system_clock::time_point
MakeUTC(int year, unsigned month, unsigned day,
        unsigned hour=0, unsigned minute=0) noexcept
{
  return sys_days{std::chrono::year{year} / std::chrono::month{month} /
                  std::chrono::day{day}} + hours{hour} + minutes{minute};
}

/**
 * Returns the UTC offset of the given POSIX TZ string at the given
 * point in time, or "invalid" if the string is malformed.
 */
static constexpr seconds INVALID{-1};

static seconds
GetOffset(const char *posix, system_clock::time_point t) noexcept
{
  const auto tz = PosixTimeZone::Parse(posix);
  return tz ? tz->GetOffset(t) : INVALID;
}

static void
TestWithoutDaylightSaving()
{
  static constexpr const char *tokyo = "JST-9";
  ok1(GetOffset(tokyo, MakeUTC(2026, 1, 15)) == hours{9});
  ok1(GetOffset(tokyo, MakeUTC(2026, 7, 15)) == hours{9});

  /* a quoted abbreviation with a fractional offset */
  static constexpr const char *kathmandu = "<+0545>-5:45";
  ok1(GetOffset(kathmandu, MakeUTC(2026, 1, 15)) == hours{5} + minutes{45});

  /* west of UTC */
  ok1(GetOffset("MST7", MakeUTC(2026, 7, 15)) == -hours{7});

  ok1(GetOffset("UTC0", MakeUTC(2026, 7, 15)) == seconds{0});
}

static void
TestNorthernHemisphere()
{
  /* Europe/Berlin: the transitions happen at 01:00 UTC, written as
     02:00 standard time in March and 03:00 daylight saving time in
     October */
  static constexpr const char *berlin = "CET-1CEST,M3.5.0,M10.5.0/3";

  ok1(GetOffset(berlin, MakeUTC(2026, 1, 15)) == hours{1});
  ok1(GetOffset(berlin, MakeUTC(2026, 7, 15)) == hours{2});

  /* 2026-03-29 is the last Sunday in March */
  ok1(GetOffset(berlin, MakeUTC(2026, 3, 29, 0, 59)) == hours{1});
  ok1(GetOffset(berlin, MakeUTC(2026, 3, 29, 1, 0)) == hours{2});

  /* 2026-10-25 is the last Sunday in October */
  ok1(GetOffset(berlin, MakeUTC(2026, 10, 25, 0, 59)) == hours{2});
  ok1(GetOffset(berlin, MakeUTC(2026, 10, 25, 1, 0)) == hours{1});

  /* a year in which the last Sunday in March is not the fifth one */
  ok1(GetOffset(berlin, MakeUTC(2027, 3, 28, 0, 59)) == hours{1});
  ok1(GetOffset(berlin, MakeUTC(2027, 3, 28, 1, 0)) == hours{2});

  /* America/New_York uses the second Sunday in March and the first
     Sunday in November */
  static constexpr const char *new_york = "EST5EDT,M3.2.0,M11.1.0";

  ok1(GetOffset(new_york, MakeUTC(2026, 1, 15)) == -hours{5});
  ok1(GetOffset(new_york, MakeUTC(2026, 7, 15)) == -hours{4});
  ok1(GetOffset(new_york, MakeUTC(2026, 3, 8, 6, 59)) == -hours{5});
  ok1(GetOffset(new_york, MakeUTC(2026, 3, 8, 7, 0)) == -hours{4});
  ok1(GetOffset(new_york, MakeUTC(2026, 11, 1, 5, 59)) == -hours{4});
  ok1(GetOffset(new_york, MakeUTC(2026, 11, 1, 6, 0)) == -hours{5});

  /* Europe/London: standard time is UTC, and the transition time is
     given explicitly for March only */
  static constexpr const char *london = "GMT0BST,M3.5.0/1,M10.5.0";

  ok1(GetOffset(london, MakeUTC(2026, 1, 15)) == hours{0});
  ok1(GetOffset(london, MakeUTC(2026, 7, 15)) == hours{1});
  ok1(GetOffset(london, MakeUTC(2026, 3, 29, 0, 59)) == hours{0});
  ok1(GetOffset(london, MakeUTC(2026, 3, 29, 1, 0)) == hours{1});
}

static void
TestSouthernHemisphere()
{
  /* Pacific/Auckland: daylight saving time spans the turn of the
     year */
  static constexpr const char *auckland = "NZST-12NZDT,M9.5.0,M4.1.0/3";

  ok1(GetOffset(auckland, MakeUTC(2026, 1, 15)) == hours{13});
  ok1(GetOffset(auckland, MakeUTC(2026, 7, 15)) == hours{12});

  /* 2026-09-27 is the last Sunday in September, 02:00 standard time */
  ok1(GetOffset(auckland, MakeUTC(2026, 9, 26, 13, 59)) == hours{12});
  ok1(GetOffset(auckland, MakeUTC(2026, 9, 26, 14, 0)) == hours{13});

  /* 2026-04-05 is the first Sunday in April, 03:00 daylight saving
     time */
  ok1(GetOffset(auckland, MakeUTC(2026, 4, 4, 13, 59)) == hours{13});
  ok1(GetOffset(auckland, MakeUTC(2026, 4, 4, 14, 0)) == hours{12});

  /* Australia/Lord_Howe shifts by half an hour only */
  static constexpr const char *lord_howe =
    "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0";

  ok1(GetOffset(lord_howe, MakeUTC(2026, 1, 15)) == hours{11});
  ok1(GetOffset(lord_howe, MakeUTC(2026, 7, 15)) == hours{10} + minutes{30});
}

static void
TestJulianRules()
{
  /* "Jn" never counts February 29, so J60 is March 1 in every year */
  static constexpr const char *julian = "XXX0YYY,J60/0,J300/0";

  ok1(GetOffset(julian, MakeUTC(2026, 2, 28, 23)) == hours{0});
  ok1(GetOffset(julian, MakeUTC(2026, 3, 1, 0)) == hours{1});

  /* 2028 is a leap year, and February 29 must not shift the date */
  ok1(GetOffset(julian, MakeUTC(2028, 2, 29, 23)) == hours{0});
  ok1(GetOffset(julian, MakeUTC(2028, 3, 1, 0)) == hours{1});

  /* the zero-based form does count February 29: day 59 is March 1 in a
     non-leap year, but February 29 in a leap year */
  static constexpr const char *zero_based = "XXX0YYY,59/0,300/0";

  ok1(GetOffset(zero_based, MakeUTC(2026, 3, 1, 0)) == hours{1});
  ok1(GetOffset(zero_based, MakeUTC(2028, 2, 29, 0)) == hours{1});
}

static void
TestMalformed()
{
  /* no abbreviation */
  ok1(!PosixTimeZone::Parse(""));
  ok1(!PosixTimeZone::Parse("1"));
  ok1(!PosixTimeZone::Parse("CE-1"));

  /* no offset */
  ok1(!PosixTimeZone::Parse("CET"));

  /* daylight saving time without rules is implementation-defined */
  ok1(!PosixTimeZone::Parse("CET-1CEST"));

  /* incomplete or malformed rules */
  ok1(!PosixTimeZone::Parse("CET-1CEST,M3.5.0"));
  ok1(!PosixTimeZone::Parse("CET-1CEST,M3.5.0,"));
  ok1(!PosixTimeZone::Parse("CET-1CEST,M13.5.0,M10.5.0"));
  ok1(!PosixTimeZone::Parse("CET-1CEST,M3.6.0,M10.5.0"));
  ok1(!PosixTimeZone::Parse("CET-1CEST,M3.5.7,M10.5.0"));
  ok1(!PosixTimeZone::Parse("CET-1CEST,J0,M10.5.0"));
  ok1(!PosixTimeZone::Parse("CET-1CEST,M3.5.0,M10.5.0/3,"));

  /* trailing garbage */
  ok1(!PosixTimeZone::Parse("CET-1x"));
}

static void
TestTable()
{
  ok1(FindTimeZone("Europe/Berlin") != nullptr);
  ok1(FindTimeZone("UTC") != nullptr);
  ok1(FindTimeZone("Nowhere/Special") == nullptr);
  ok1(FindTimeZone("") == nullptr);

  ok1(FindTimeZoneOffset("Europe/Berlin", MakeUTC(2026, 1, 15)) == hours{1});
  ok1(FindTimeZoneOffset("Europe/Berlin", MakeUTC(2026, 7, 15)) == hours{2});
  ok1(FindTimeZoneOffset("UTC", MakeUTC(2026, 7, 15)) == seconds{0});
  ok1(!FindTimeZoneOffset("Nowhere/Special", MakeUTC(2026, 7, 15)));

  /* every entry must be sorted, unique and parseable, and must yield a
     plausible offset */
  const auto table = GetTimeZones();
  ok1(!table.empty());

  bool ok = true;
  for (std::size_t i = 0; i < table.size(); ++i) {
    const auto &entry = table[i];

    if (i > 0 && std::string_view{table[i - 1].id} >= entry.id)
      ok = false;

    const auto tz = PosixTimeZone::Parse(entry.posix);
    if (!tz) {
      ok = false;
      continue;
    }

    for (const auto t : {MakeUTC(2026, 1, 15), MakeUTC(2026, 7, 15)}) {
      const auto offset = tz->GetOffset(t);
      if (offset < -hours{13} || offset > hours{14})
        ok = false;
    }
  }

  ok1(ok);
}

int
main()
{
  plan_tests(60);

  TestWithoutDaylightSaving();
  TestNorthernHemisphere();
  TestSouthernHemisphere();
  TestJulianRules();
  TestMalformed();
  TestTable();

  return exit_status();
}
