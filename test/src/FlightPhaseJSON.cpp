/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Util/StaticString.hxx"
#include "JSON/Writer.hpp"
#include "JSON/GeoWriter.hpp"

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

static void
WritePhase(BufferedOutputStream &writer, Phase &phase)
{
  JSON::ObjectWriter object(writer);
  NarrowString<64> buffer;

  FormatISO8601(buffer.buffer(), phase.start_datetime);
  object.WriteElement("start_time", JSON::WriteString, buffer);

  FormatISO8601(buffer.buffer(), phase.end_datetime);
  object.WriteElement("end_time", JSON::WriteString, buffer);

  object.WriteElement("type", JSON::WriteString,
                      FormatPhaseType(phase.phase_type));
  object.WriteElement("duration", JSON::WriteInteger, (int)phase.duration);
  object.WriteElement("circling_direction", JSON::WriteString,
                      FormatCirclingDirection(phase.circling_direction));
  object.WriteElement("alt_diff", JSON::WriteInteger, (int)phase.alt_diff);
  object.WriteElement("start_alt", JSON::WriteDouble, phase.start_alt);
  object.WriteElement("end_alt", JSON::WriteDouble, phase.end_alt);
  object.WriteElement("distance", JSON::WriteInteger, (int)phase.distance);
  object.WriteElement("speed", JSON::WriteDouble, phase.GetSpeed());
  object.WriteElement("vario", JSON::WriteDouble, phase.GetVario());
  object.WriteElement("glide_rate", JSON::WriteDouble, phase.GetGlideRate());
}

static void
WriteCirclingStats(BufferedOutputStream &writer, const Phase &stats)
{
  JSON::ObjectWriter object(writer);
  object.WriteElement("alt_diff", JSON::WriteInteger, (int)stats.alt_diff);
  object.WriteElement("duration", JSON::WriteInteger, (int)stats.duration);
  object.WriteElement("fraction", JSON::WriteDouble, stats.fraction);
  object.WriteElement("vario", JSON::WriteDouble, stats.GetVario());
  object.WriteElement("count", JSON::WriteInteger, stats.merges);
}

static void
WriteCruiseStats(BufferedOutputStream &writer, const Phase &stats)
{
  JSON::ObjectWriter object(writer);
  object.WriteElement("alt_diff", JSON::WriteInteger, (int)stats.alt_diff);
  object.WriteElement("duration", JSON::WriteInteger, (int)stats.duration);
  object.WriteElement("fraction", JSON::WriteDouble, stats.fraction);
  object.WriteElement("distance", JSON::WriteInteger, (int)stats.distance);
  object.WriteElement("speed", JSON::WriteDouble, stats.GetSpeed());
  object.WriteElement("vario", JSON::WriteDouble, stats.GetVario());
  object.WriteElement("glide_rate", JSON::WriteDouble, stats.GetGlideRate());
  object.WriteElement("start_alt", JSON::WriteDouble, stats.start_alt);
  object.WriteElement("end_alt", JSON::WriteDouble, stats.end_alt);
  object.WriteElement("count", JSON::WriteInteger, stats.merges);
}

void
WritePerformanceStats(BufferedOutputStream &writer, const PhaseTotals &totals)
{
  JSON::ObjectWriter object(writer);
  object.WriteElement("circling_total", WriteCirclingStats,
                      totals.total_circstats);
  object.WriteElement("circling_left", WriteCirclingStats,
                      totals.left_circstats);
  object.WriteElement("circling_right", WriteCirclingStats,
                      totals.right_circstats);
  object.WriteElement("circling_mixed", WriteCirclingStats,
                      totals.mixed_circstats);
  object.WriteElement("cruise_total", WriteCruiseStats,
                      totals.total_cruisestats);
}

void
WritePhaseList(BufferedOutputStream &writer, const PhaseList &phases)
{
  JSON::ArrayWriter array(writer);
  for (Phase phase : phases) {
    array.WriteElement(WritePhase, phase);
  }
}

