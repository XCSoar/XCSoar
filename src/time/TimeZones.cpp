// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TimeZones.hpp"
#include "PosixTimeZone.hpp"

const TimeZoneEntry *
FindTimeZone(std::string_view id) noexcept
{
  for (const auto &i : GetTimeZones())
    if (id == i.id)
      return &i;

  return nullptr;
}

std::optional<std::chrono::seconds>
FindTimeZoneOffset(std::string_view id,
                   std::chrono::system_clock::time_point t) noexcept
{
  const auto *entry = FindTimeZone(id);
  if (entry == nullptr)
    return std::nullopt;

  const auto tz = PosixTimeZone::Parse(entry->posix);
  if (!tz)
    return std::nullopt;

  return tz->GetOffset(t);
}
