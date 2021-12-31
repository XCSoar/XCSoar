/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "FlightPhaseJSON.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "util/StaticString.hxx"
#include "json/Geo.hpp"

#include <boost/json.hpp>

static const char *
FormatPhaseType(Phase::Type phase_type)
{
  switch (phase_type) {
  case Phase::Type::CRUISE:
    return "cruise";
  case Phase::Type::CIRCLING:
    return "circling";
  case Phase::Type::POWERED:
    return "powered";
  default:
    return "";
  }
}

static const char *
FormatCirclingDirection(Phase::CirclingDirection circling_direction)
{
  switch (circling_direction) {
  case Phase::CirclingDirection::LEFT:
    return "left";
  case Phase::CirclingDirection::RIGHT:
    return "right";
  case Phase::CirclingDirection::MIXED:
    return "mixed";
  default:
    return "";
  }
}

static boost::json::object
WritePhase(Phase &phase) noexcept
{
  boost::json::object object;
  NarrowString<64> buffer;

  FormatISO8601(buffer.buffer(), phase.start_datetime);
  object.emplace("start_time", buffer.c_str());

  FormatISO8601(buffer.buffer(), phase.end_datetime);
  object.emplace("end_time", buffer.c_str());

  object.emplace("type", FormatPhaseType(phase.phase_type));
  object.emplace("duration", (int)phase.duration.count());
  object.emplace("circling_direction",
                 FormatCirclingDirection(phase.circling_direction));
  object.emplace("alt_diff", (int)phase.alt_diff);
  object.emplace("start_alt", phase.start_alt);
  object.emplace("end_alt", phase.end_alt);
  object.emplace("distance", (int)phase.distance);
  object.emplace("speed", phase.GetSpeed());
  object.emplace("vario", phase.GetVario());
  object.emplace("glide_rate", phase.GetGlideRate());

  return object;
}

static boost::json::object
WriteCirclingStats(const Phase &stats) noexcept
{
  return {
    {"alt_diff", (int)stats.alt_diff},
    {"duration", (int)stats.duration.count()},
    {"fraction", stats.fraction},
    {"vario", stats.GetVario()},
    {"count", stats.merges},
  };
}

static boost::json::object
WriteCruiseStats(const Phase &stats) noexcept
{
  return {
    {"alt_diff", (int)stats.alt_diff},
    {"duration", (int)stats.duration.count()},
    {"fraction", stats.fraction},
    {"distance", (int)stats.distance},
    {"speed", stats.GetSpeed()},
    {"vario", stats.GetVario()},
    {"glide_rate", stats.GetGlideRate()},
    {"start_alt", stats.start_alt},
    {"end_alt", stats.end_alt},
    {"count", stats.merges},
  };
}

boost::json::object
WritePerformanceStats(const PhaseTotals &totals) noexcept
{
  return {
    {"circling_total", WriteCirclingStats(totals.total_circstats)},
    {"circling_left", WriteCirclingStats(totals.left_circstats)},
    {"circling_right", WriteCirclingStats(totals.right_circstats)},
    {"circling_mixed", WriteCirclingStats(totals.mixed_circstats)},
    {"cruise_total", WriteCruiseStats(totals.total_cruisestats)},
  };
}

boost::json::array
WritePhaseList(const PhaseList &phases) noexcept
{
  boost::json::array array;
  for (Phase phase : phases) {
    array.emplace_back(WritePhase(phase));
  }

  return array;
}
