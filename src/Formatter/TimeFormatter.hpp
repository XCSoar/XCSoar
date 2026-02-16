// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"
#include "util/StringBuffer.hxx"

#include <tchar.h>

struct BrokenDate;
struct BrokenDateTime;

void
FormatISO8601(char *buffer, const BrokenDate &date) noexcept;

/**
 * Format a UTC time stamp in ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ).
 */
void
FormatISO8601(char *buffer, const BrokenDateTime &stamp) noexcept;

void
FormatTime(char *buffer, FloatDuration time) noexcept;

static inline void
FormatTime(char *buffer, TimeStamp time) noexcept
{
  FormatTime(buffer, time.ToDuration());
}

void
FormatTimeLong(char *buffer, FloatDuration time) noexcept;

/**
 * precedes with "-" if time is negative
 * @param buffer returns HHMM
 * @param time input seconds
 */
void
FormatSignedTimeHHMM(char *buffer, std::chrono::seconds time) noexcept;

[[gnu::const]]
static inline BasicStringBuffer<char, 8>
FormatSignedTimeHHMM(std::chrono::seconds time) noexcept
{
  BasicStringBuffer<char, 8> buffer;
  FormatSignedTimeHHMM(buffer.data(), time);
  return buffer;
}

[[gnu::const]]
static inline auto
FormatSignedTimeHHMM(FloatDuration time) noexcept
{
  return FormatSignedTimeHHMM(std::chrono::duration_cast<std::chrono::seconds>(time));
}

static inline void
FormatTimeHHMM(char *buffer, TimeStamp time) noexcept
{
  FormatSignedTimeHHMM(buffer, time.Cast<std::chrono::seconds>());
}

[[gnu::const]]
static inline auto
FormatTimeHHMM(std::chrono::duration<unsigned> time) noexcept
{
  return FormatSignedTimeHHMM(std::chrono::duration_cast<std::chrono::seconds>(time));
}

[[gnu::const]]
static inline auto
FormatTimeHHMM(TimeStamp time) noexcept
{
  return FormatSignedTimeHHMM(time.Cast<std::chrono::seconds>());
}

/**
 * sets HHMMSSSmart and SSSmart
 * if hours > 0, returns HHMM in buffer1 and SS in buffer2
 * if hours == 0, returns MMSS in buffer1 and "" in buffer2
 * @param d input seconds
 */
void
FormatTimeTwoLines(char *buffer1, char *buffer2,
                   std::chrono::seconds time) noexcept;

void
FormatTimespanSmart(char *buffer, std::chrono::seconds timespan,
                    unsigned max_tokens = 1,
                    const char *separator = " ") noexcept;

[[gnu::const]]
static inline BasicStringBuffer<char, 64>
FormatTimespanSmart(std::chrono::seconds timespan, unsigned max_tokens = 1,
                    const char *separator = " ") noexcept
{
  BasicStringBuffer<char, 64> buffer;
  FormatTimespanSmart(buffer.data(), timespan, max_tokens, separator);
  return buffer;
}

[[gnu::const]]
static inline auto
FormatTimespanSmart(FloatDuration timespan, unsigned max_tokens = 1,
                    const char *separator = " ") noexcept
{
  return FormatTimespanSmart(std::chrono::duration_cast<std::chrono::seconds>(timespan),
                             max_tokens, separator);
}
