// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BrokenDateTime.hpp"
#include "Calendar.hxx"
#include "Convert.hxx"

#include <cassert>

#include <chrono>
#include <cstdint>
#include <time.h>

static BrokenDateTime
ToBrokenDateTimeUtcSeconds(int64_t t) noexcept
{
  /* Build UTC civil time from a signed second count, without converting through
   * std::time_t (so values beyond 32-bit time_t on ILP32 are not truncated). */
  using std::chrono::days;
  using std::chrono::hh_mm_ss;
  using std::chrono::local_days;
  using std::chrono::seconds;
  using std::chrono::sys_days;
  using std::chrono::sys_seconds;
  using std::chrono::weekday;
  using std::chrono::year_month_day;

  const sys_seconds ss{seconds(t)};
  const year_month_day ymd{sys_days{std::chrono::floor<days>(ss)}};
  const auto since_midnight = ss - sys_seconds{sys_days{ymd}};
  const hh_mm_ss<seconds> hms{since_midnight};

  BrokenDateTime dt;
  dt.year = static_cast<unsigned>(static_cast<int>(ymd.year()));
  dt.month = static_cast<unsigned>(ymd.month());
  dt.day = static_cast<unsigned>(ymd.day());
  dt.day_of_week = static_cast<int8_t>(weekday{local_days{ymd}}.c_encoding());
  dt.hour = static_cast<unsigned>(hms.hours().count());
  dt.minute = static_cast<unsigned>(hms.minutes().count());
  dt.second = static_cast<unsigned>(hms.seconds().count());
  return dt;
}

static const BrokenDateTime
ToBrokenDateTime(const struct tm &tm) noexcept
{
  BrokenDateTime dt;

  dt.year = tm.tm_year + 1900;
  dt.month = tm.tm_mon + 1;
  dt.day = tm.tm_mday;
  dt.day_of_week = tm.tm_wday;

  dt.hour = tm.tm_hour;
  dt.minute = tm.tm_min;
  dt.second = tm.tm_sec;

  return dt;
}

BrokenDateTime::BrokenDateTime(std::chrono::system_clock::time_point tp) noexcept
  : BrokenDateTime(FromUnixTime(
        (int64_t)std::chrono::duration_cast<std::chrono::seconds>(
            tp.time_since_epoch())
            .count())) {
}

std::chrono::system_clock::time_point
BrokenDateTime::ToTimePoint() const noexcept
{
  assert(IsPlausible());

  struct tm tm;
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;
  tm.tm_isdst = 0;
  tm.tm_wday = day_of_week;
  tm.tm_yday = -1;

  return TimeGm(tm);
}

const BrokenDateTime
BrokenDateTime::NowUTC() noexcept
{
  return BrokenDateTime{std::chrono::system_clock::now()};
}

const BrokenDateTime
BrokenDateTime::NowLocal() noexcept
{
  return ToBrokenDateTime(LocalTime(std::chrono::system_clock::now()));
}

BrokenDateTime
BrokenDateTime::FromUnixTime(int64_t t) noexcept
{
  return ToBrokenDateTimeUtcSeconds(t);
}
