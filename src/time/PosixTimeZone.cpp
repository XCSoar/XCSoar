// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PosixTimeZone.hpp"
#include "util/CharUtil.hxx"

using namespace std::chrono;

/**
 * The largest value POSIX allows in an offset field.
 */
static constexpr unsigned MAX_OFFSET_HOURS = 24;

/**
 * Transition times may reach into the neighbouring days, and glibc
 * allows a whole week in either direction.
 */
static constexpr unsigned MAX_TIME_HOURS = 167;

/**
 * Skip the time zone abbreviation at the beginning of the given
 * string; it is of no use to us, because we never print it.
 *
 * @return false if there is no (well-formed) abbreviation
 */
static bool
SkipAbbreviation(std::string_view &s) noexcept
{
  if (s.starts_with('<')) {
    /* the quoted form allows digits and signs, e.g. "<+07>" */
    const auto end = s.find('>');
    if (end == s.npos || end < 4)
      return false;

    s.remove_prefix(end + 1);
    return true;
  }

  std::size_t length = 0;
  while (length < s.size() && IsAlphaASCII(s[length]))
    ++length;

  if (length < 3)
    return false;

  s.remove_prefix(length);
  return true;
}

static std::optional<unsigned>
ParseUnsigned(std::string_view &s, std::size_t max_digits) noexcept
{
  unsigned value = 0;
  std::size_t length = 0;

  while (length < s.size() && length < max_digits && IsDigitASCII(s[length])) {
    value = value * 10 + (s[length] - '0');
    ++length;
  }

  if (length == 0)
    return std::nullopt;

  s.remove_prefix(length);
  return value;
}

/**
 * Parse "[+|-]hh[:mm[:ss]]".
 */
static std::optional<seconds>
ParseOffset(std::string_view &s, unsigned max_hours) noexcept
{
  bool negative = false;
  if (s.starts_with('+') || s.starts_with('-')) {
    negative = s.starts_with('-');
    s.remove_prefix(1);
  }

  const auto h = ParseUnsigned(s, 3);
  if (!h || *h > max_hours)
    return std::nullopt;

  unsigned m = 0, sec = 0;

  if (s.starts_with(':')) {
    s.remove_prefix(1);

    const auto value = ParseUnsigned(s, 2);
    if (!value || *value > 59)
      return std::nullopt;

    m = *value;

    if (s.starts_with(':')) {
      s.remove_prefix(1);

      const auto value2 = ParseUnsigned(s, 2);
      if (!value2 || *value2 > 59)
        return std::nullopt;

      sec = *value2;
    }
  }

  const seconds value = hours{*h} + minutes{m} + seconds{sec};
  return negative ? -value : value;
}

/**
 * Parse "Mm.w.d", "Jn" or "n", each optionally followed by "/time".
 */
static std::optional<PosixTimeZone::Transition>
ParseTransition(std::string_view &s) noexcept
{
  using Transition = PosixTimeZone::Transition;
  using Format = Transition::Format;

  Transition transition{};

  if (s.starts_with('M')) {
    s.remove_prefix(1);

    const auto month = ParseUnsigned(s, 2);
    if (!month || *month < 1 || *month > 12 || !s.starts_with('.'))
      return std::nullopt;

    s.remove_prefix(1);

    const auto week = ParseUnsigned(s, 1);
    if (!week || *week < 1 || *week > 5 || !s.starts_with('.'))
      return std::nullopt;

    s.remove_prefix(1);

    const auto day_of_week = ParseUnsigned(s, 1);
    if (!day_of_week || *day_of_week > 6)
      return std::nullopt;

    transition.format = Format::MONTH_WEEK_DAY;
    transition.month = *month;
    transition.week = *week;
    transition.day_of_week = *day_of_week;
  } else if (s.starts_with('J')) {
    s.remove_prefix(1);

    const auto day = ParseUnsigned(s, 3);
    if (!day || *day < 1 || *day > 365)
      return std::nullopt;

    transition.format = Format::JULIAN;
    transition.day = *day;
  } else {
    const auto day = ParseUnsigned(s, 3);
    if (!day || *day > 365)
      return std::nullopt;

    transition.format = Format::ZERO_BASED_JULIAN;
    transition.day = *day;
  }

  /* POSIX specifies 02:00:00 as the default */
  transition.time = hours{2};

  if (s.starts_with('/')) {
    s.remove_prefix(1);

    const auto time = ParseOffset(s, MAX_TIME_HOURS);
    if (!time)
      return std::nullopt;

    transition.time = *time;
  }

  return transition;
}

std::optional<PosixTimeZone>
PosixTimeZone::Parse(std::string_view s) noexcept
{
  PosixTimeZone tz;

  if (!SkipAbbreviation(s))
    return std::nullopt;

  const auto standard_offset = ParseOffset(s, MAX_OFFSET_HOURS);
  if (!standard_offset)
    return std::nullopt;

  /* POSIX specifies the value which must be added to local time to
     obtain UTC, which is the opposite of what everybody else writes */
  tz.standard_offset = -*standard_offset;

  if (s.empty())
    /* no daylight saving time in this zone */
    return tz;

  if (!SkipAbbreviation(s))
    return std::nullopt;

  if (s.empty() || s.starts_with(',')) {
    /* without an explicit offset, daylight saving time is one hour
       ahead of standard time */
    tz.daylight_offset = tz.standard_offset + hours{1};
  } else {
    const auto daylight_offset = ParseOffset(s, MAX_OFFSET_HOURS);
    if (!daylight_offset)
      return std::nullopt;

    tz.daylight_offset = -*daylight_offset;
  }

  /* POSIX allows omitting the rules, but then the transition dates are
     implementation-defined; we refuse to guess, and the zoneinfo
     database never does that */
  if (!s.starts_with(','))
    return std::nullopt;

  s.remove_prefix(1);

  const auto daylight_start = ParseTransition(s);
  if (!daylight_start || !s.starts_with(','))
    return std::nullopt;

  s.remove_prefix(1);

  const auto daylight_end = ParseTransition(s);
  if (!daylight_end || !s.empty())
    return std::nullopt;

  tz.daylight_start = *daylight_start;
  tz.daylight_end = *daylight_end;
  tz.has_daylight_saving = true;

  return tz;
}

sys_days
PosixTimeZone::Transition::GetDate(year y) const noexcept
{
  switch (format) {
  case Format::MONTH_WEEK_DAY:
    if (week >= 5)
      /* "5" does not mean the fifth one, it always means the last
         one */
      return sys_days{y / std::chrono::month{month} /
                      weekday{day_of_week}[last]};

    return sys_days{y / std::chrono::month{month} /
                    weekday{day_of_week}[week]};

  case Format::JULIAN:
    {
      /* February 29 is never counted, i.e. day 60 is March 1 in every
         year */
      auto date = sys_days{y / January / 1} + days{day - 1};
      if (day >= 60 && y.is_leap())
        date += days{1};
      return date;
    }

  case Format::ZERO_BASED_JULIAN:
    return sys_days{y / January / 1} + days{day};
  }

  /* unreachable */
  return sys_days{y / January / 1};
}

seconds
PosixTimeZone::GetOffset(system_clock::time_point t) const noexcept
{
  if (!has_daylight_saving)
    return standard_offset;

  const auto utc = floor<seconds>(t);

  /* determine the year in local standard time; a transition never
     happens close enough to New Year for the difference between
     standard and daylight saving time to select the wrong one */
  const year_month_day date{floor<days>(utc + standard_offset)};

  /* POSIX specifies both transition times in the local time which is
     in effect immediately before the respective transition */
  const auto start = daylight_start.GetDate(date.year()) +
    daylight_start.time - standard_offset;
  const auto end = daylight_end.GetDate(date.year()) +
    daylight_end.time - daylight_offset;

  const bool daylight = start <= end
    /* northern hemisphere */
    ? utc >= start && utc < end
    /* southern hemisphere: daylight saving time spans the turn of the
       year */
    : utc >= start || utc < end;

  return daylight ? daylight_offset : standard_offset;
}
