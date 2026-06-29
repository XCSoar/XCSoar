// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TrafficDisplay.hpp"
#include "TrafficExtensions.hpp"
#include "Language/Language.hpp"
#include "util/StringCompare.hxx"
#include "util/UTF8.hpp"

namespace SkyLinesTracking {

template<std::size_t max>
void
FormatTrafficTitle(StaticString<max> &dest, uint32_t pilot_id,
                   FlarmId flarm_id, const char *server_name,
                   const char *live_name) noexcept
{
  dest.clear();

  if (flarm_id.IsDefined()) {
    const ResolvedInfo info = FlarmDetails::ResolveInfo(flarm_id);

    if (!info.callsign.empty() && dest.SetUTF8(info.callsign.c_str()))
      return;

    if (!info.registration.empty() && dest.SetUTF8(info.registration.c_str()))
      return;

    if (!info.pilot.empty() && dest.SetUTF8(info.pilot.c_str()))
      return;
  }

  if (live_name != nullptr && !StringIsEmpty(live_name) &&
      dest.SetUTF8(live_name))
    return;

  if (server_name != nullptr && !StringIsEmpty(server_name) &&
      dest.SetUTF8(server_name))
    return;

  if ((pilot_id & OGN_PILOT_ID_MASK) == 0)
    dest.UnsafeFormat(_("Online traffic %u"), unsigned(pilot_id));
}

} // namespace SkyLinesTracking
