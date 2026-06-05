// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"

#include <string_view>

namespace WaypointDetails {

[[gnu::pure]]
static inline bool
IsSectionHeader(std::string_view line) noexcept
{
  if (line.size() < 3 || line.front() != '[')
    return false;

  const auto end = line.find(']');
  return end != std::string_view::npos && end > 1;
}

[[gnu::pure]]
static inline bool
IsAttachmentLine(std::string_view line) noexcept
{
  line = Strip(line);
  return StringStartsWithIgnoreCase(line, "image=") ||
         StringStartsWithIgnoreCase(line, "file=");
}

} // namespace WaypointDetails
