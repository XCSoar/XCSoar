/*
Copyright_License {

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

#include "Index.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "Resources.hpp"

#include <stdlib.h>
#include <tchar.h>

typedef struct {
  const char *name;
  ResourceId resource_id, big_resource_id;
} LOOKUP_ICON;

static constexpr LOOKUP_ICON icon_list[] = {
  { "landable", IDB_LANDABLE, IDB_LANDABLE_HD },
  { "reachable", IDB_REACHABLE, IDB_REACHABLE_HD },
  { "turnpoint", IDB_TURNPOINT, IDB_TURNPOINT_HD },
  { "small", IDB_SMALL, IDB_SMALL_HD },
  { "cruise", IDB_CRUISE, IDB_CRUISE_HD },
  { "town", IDB_TOWN, IDB_TOWN_HD },
  { "mark", IDB_MARK, IDB_MARK_HD },
  { "terrainwarning", IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD },
  { "airport_reachable", IDB_AIRPORT_REACHABLE, IDB_AIRPORT_REACHABLE_HD },
  { "airport_unreachable",
    IDB_AIRPORT_UNREACHABLE, IDB_AIRPORT_UNREACHABLE_HD },
  { "outfield_reachable", IDB_OUTFIELD_REACHABLE, IDB_OUTFIELD_REACHABLE_HD },
  { "outfield_reachable",
    IDB_OUTFIELD_UNREACHABLE, IDB_OUTFIELD_UNREACHABLE_HD },
  { "target", IDB_TARGET, IDB_TARGET_HD },
  { "teammate_pos", IDB_TEAMMATE_POS, IDB_TEAMMATE_POS_HD },
  { "airport_unreachable2",
    IDB_AIRPORT_UNREACHABLE2, IDB_AIRPORT_UNREACHABLE2_HD },
  { "outfield_unreachable2",
    IDB_OUTFIELD_UNREACHABLE2, IDB_OUTFIELD_UNREACHABLE2_HD },
  { "airspacei", IDB_AIRSPACEI, IDB_AIRSPACEI_HD },
  { "mountain_top", IDB_MOUNTAIN_TOP, IDB_MOUNTAIN_TOP_HD },
  { "bridge", IDB_BRIDGE, IDB_BRIDGE_HD },
  { "tunnel", IDB_TUNNEL, IDB_TUNNEL_HD },
  { "tower", IDB_TOWER, IDB_TOWER_HD },
  { "power_plant", IDB_POWER_PLANT, IDB_POWER_PLANT_HD },
  { "airport_marginal", IDB_AIRPORT_MARGINAL, IDB_AIRPORT_MARGINAL_HD },
  { "outfield_marginal", IDB_OUTFIELD_MARGINAL, IDB_OUTFIELD_MARGINAL_HD },
  { "airport_marginal2", IDB_AIRPORT_MARGINAL2, IDB_AIRPORT_MARGINAL2_HD },
  { "outfield_marginal2", IDB_OUTFIELD_MARGINAL2, IDB_OUTFIELD_MARGINAL2_HD },
  { "marginal", IDB_MARGINAL, IDB_MARGINAL_HD },
  { "traffic_safe", IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD },
  { "traffic_warning", IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD },
  { "traffic_alarm", IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD },
  { "taskturnpoint", IDB_TASKTURNPOINT, IDB_TASKTURNPOINT_HD },
  { "obstacle", IDB_OBSTACLE, IDB_OBSTACLE_HD },
  { "mountain_pass", IDB_MOUNTAIN_PASS, IDB_MOUNTAIN_PASS_HD },
  { "weather_station", IDB_WEATHER_STATION, IDB_WEATHER_STATION_HD },
  { "thermal_hotspot", IDB_THERMAL_HOTSPOT, IDB_THERMAL_HOTSPOT_HD },
  { nullptr, ResourceId::Null(), ResourceId::Null() }
};

std::optional<TopographyIndexEntry>
ParseTopographyIndexLine(const char *line) noexcept
{
  // .tpl Line format: filename,range,icon,field,r,g,b,pen_width,label_range,important_range,alpha

  TopographyIndexEntry entry;

    // Ignore comments (lines starting with *) and empty lines
    if (StringIsEmpty(line) || line[0] == '*')
      return std::nullopt;

    // Find first comma to extract shape filename
    const char *p = strchr(line, ',');
    if (p == nullptr || p == line)
      // If no comma was found -> ignore this line/shapefile
      return std::nullopt;

    entry.name = {line, std::size_t(p - line)};

    // Parse shape range
    char *endptr;
    entry.shape_range = strtod(p + 1, &endptr) * 1000;
    if (*endptr != _T(','))
      return std::nullopt;

    p = endptr + 1;

    // Extract shape icon name
    const char *start = p;
    p = strchr(start, ',');
    if (p == nullptr)
      return std::nullopt;

    const std::string_view icon_name{start, std::size_t(p - start)};

    entry.icon = ResourceId::Null();
    entry.big_icon = ResourceId::Null();
    if (!icon_name.empty()) {
      const LOOKUP_ICON *ip = icon_list;
      while (ip->name != nullptr) {
        if (icon_name == ip->name) {
          entry.icon = ip->resource_id;
          entry.big_icon = ip->big_resource_id;
          break;
        }
        ip++;
      }
    }

    // Parse shape field for text display
    entry.shape_field = strtol(p + 1, &endptr, 10) - 1;
    if (*endptr != _T(','))
      return std::nullopt;

    p = endptr + 1;

    // Parse red component of line / shading colour
    uint8_t red = (uint8_t)strtol(p, &endptr, 10);
    if (*endptr != _T(','))
      return std::nullopt;

    p = endptr + 1;

    // Parse green component of line / shading colour
    uint8_t green = (uint8_t)strtol(p, &endptr, 10);
    if (*endptr != _T(','))
      return std::nullopt;

    p = endptr + 1;

    // Parse blue component of line / shading colour
    uint8_t blue = (uint8_t)strtol(p, &endptr, 10);

    p = endptr;

    // Parse pen width of lines
    entry.pen_width = 1;
    if (*p == _T(',')) {
      entry.pen_width = strtoul(p + 1, &endptr, 10);
      if (entry.pen_width < 1)
        entry.pen_width = 1;
      else if (entry.pen_width > 31)
        entry.pen_width = 31;

      p = endptr;
    }

    // Parse range for displaying labels
    entry.label_range = entry.shape_range;
    if (*p == _T(',')) {
      entry.label_range = strtod(p + 1, &endptr) * 1000;
      p = endptr;
    }

    // Parse range for displaying labels with "important" rendering style
    entry.important_label_range = 0;
    if (*p == _T(',')) {
      entry.important_label_range = strtod(p + 1, &endptr) * 1000;
      p = endptr;
    }

    // Handle alpha component
    // If not present at all (i.e. v6.6 or earlier file), default to 100% opaque
    uint8_t alpha = 255;
    if (*p == _T(',')) {
      // An alpha component of shading colour is present (v6.7 or later file).
      alpha = (uint8_t)strtol(p + 1, &endptr, 10);
      // Ignore a totally transparent file!
      if (alpha == 0)
        return std::nullopt;
#ifndef ENABLE_OPENGL
      // Without OpenGL ignore anything but 100% opaque
      if (alpha != 255)
        return std::nullopt;
#endif

      p = endptr;
    }

    entry.color = {red, green, blue, alpha};

    return entry;
}
