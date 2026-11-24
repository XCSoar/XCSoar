// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CupWriter.hpp"
#include "SeeYouStyle.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "io/BufferedOutputStream.hxx"
#include "Engine/Waypoint/Runway.hpp"

/**
 * Write a quoted CSV field, escaping any embedded double quotes
 * by doubling them per RFC 4180.
 */
static void
WriteCsvQuoted(BufferedOutputStream &writer, std::string_view value)
{
  writer.Write('"');
  for (const char ch : value) {
    if (ch == '"')
      writer.Write('"');
    writer.Write(ch);
  }
  writer.Write('"');
}

static void
WriteAngleDMM(BufferedOutputStream &writer, const Angle angle, bool is_latitude)
{
  // Calculate degrees, minutes and decimal minutes
  const auto dmm = angle.ToDMM();

  // Save them into the buffer string
  writer.Fmt("{:0{}}{:02}.{:03}{}",
             dmm.degrees, is_latitude ? 2 : 3,
             dmm.minutes, dmm.decimal_minutes,
             is_latitude ? (dmm.positive ? 'N' : 'S') : (dmm.positive ? 'E' : 'W'));
}

static void
WriteAltitude(BufferedOutputStream &writer, double altitude)
{
  writer.Fmt("{}M", (int)altitude);
}

/**
 * CUP style to write. 0 means omit the field.
 * A home airfield is style 4; other airfields use the first table
 * row (2). Styles 4 and 5 are only read aliases.
 */
[[gnu::pure]]
static unsigned
GetSeeYouStyle(const Waypoint &wp) noexcept
{
  if (wp.type == Waypoint::Type::AIRFIELD && wp.flags.home)
    return 4;

  for (const auto &entry : see_you_styles)
    if (entry.type == wp.type)
      return entry.style;

  return 0;
}

void
WriteCup(BufferedOutputStream &writer, const Waypoint &wp)
{
  // Write Title
  WriteCsvQuoted(writer, wp.name);
  writer.Write(',');

  // Write Code / Short Name
  WriteCsvQuoted(writer, wp.shortname);
  writer.Write(',');

  // Write Country
  writer.Write(',');

  // Write Latitude
  WriteAngleDMM(writer, wp.location.latitude, true);
  writer.Write(',');

  // Write Longitude
  WriteAngleDMM(writer, wp.location.longitude, false);
  writer.Write(',');

  // Write Elevation
  if (wp.HasElevation())
    WriteAltitude(writer, wp.elevation);
  writer.Write(',');

  // Write Style
  if (const unsigned style = GetSeeYouStyle(wp))
    writer.Fmt("{}", style);
  writer.Write(',');

  // Write Runway Direction
  if ((wp.type == Waypoint::Type::AIRFIELD ||
       wp.type == Waypoint::Type::OUTLANDING) &&
      wp.runway.IsDirectionDefined())
    writer.Fmt("{:03}", wp.runway.GetDirectionDegrees());

  writer.Write(',');

  // Write Runway Length
  if ((wp.type == Waypoint::Type::AIRFIELD ||
       wp.type == Waypoint::Type::OUTLANDING) &&
      wp.runway.IsLengthDefined())
    writer.Fmt("{:03}M", wp.runway.GetLength());

  writer.Write(',');

  // Write Runway Width (not stored, always empty)
  writer.Write(',');

  // Write Airport Frequency
  if (wp.radio_frequency.IsDefined()) {
    const unsigned freq = wp.radio_frequency.GetKiloHertz();
    writer.Fmt("\"{}.{:03}\"", freq / 1000, freq % 1000);
  }

  writer.Write(',');

  // Write Description
  WriteCsvQuoted(writer, wp.comment);
  writer.Write(',');

  // Write Userdata (details)
  WriteCsvQuoted(writer, wp.details);
  writer.Write(',');

  // Write Pics (embedded files, semicolon-separated)
  if (!wp.files_embed.empty()) {
    std::string pics;
    for (const auto &file : wp.files_embed) {
      if (!pics.empty())
        pics += ';';
      pics += file;
    }
    WriteCsvQuoted(writer, pics);
  } else {
    writer.Write("\"\"");
  }

  writer.Write('\n');
}

void
WriteCupHeader(BufferedOutputStream &writer)
{
  writer.Write("name,code,country,lat,lon,elev,style,"
               "rwdir,rwlen,rwwidth,freq,desc,userdata,pics\n");
}

void
WriteCup(BufferedOutputStream &writer, const Waypoints &waypoints,
         WaypointOrigin origin)
{
  WriteCupHeader(writer);

  // Iterate through the waypoint list and save each waypoint with
  // the right file number to the BufferedOutputStream
  /// @todo JMW: iteration ordered by ID would be preferred
  for (auto i : waypoints) {
    const Waypoint &wp = *i;
    if (wp.origin == origin)
      WriteCup(writer, wp);
  }
}
