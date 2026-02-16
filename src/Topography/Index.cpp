// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Index.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "Resources.hpp"

#include <stdlib.h>
#include <tchar.h>

typedef struct {
  const char *name;
  ResourceId resource_id, big_resource_id, ultra_resource_id;
} LOOKUP_ICON;

static constexpr LOOKUP_ICON icon_list[] = {
  { "landable", IDB_LANDABLE_ALL },
  { "reachable", IDB_REACHABLE_ALL },
  { "turnpoint", IDB_TURNPOINT_ALL },
  { "small", IDB_SMALL_ALL },
  { "cruise", IDB_CRUISE_ALL },
  { "town", IDB_TOWN_ALL },
  { "mark", IDB_MARK_ALL },
  { "terrainwarning", IDB_TERRAINWARNING_ALL },
  { "airport_reachable", IDB_AIRPORT_REACHABLE_ALL },
  { "airport_unreachable", IDB_AIRPORT_UNREACHABLE_ALL },
  { "outfield_reachable", IDB_OUTFIELD_REACHABLE_ALL },
  { "outfield_reachable", IDB_OUTFIELD_UNREACHABLE_ALL },
  { "target", IDB_TARGET_ALL },
  { "teammate_pos", IDB_TEAMMATE_POS_ALL },
  { "airport_unreachable2", IDB_AIRPORT_UNREACHABLE2_ALL },
  { "outfield_unreachable2", IDB_OUTFIELD_UNREACHABLE2_ALL },
  { "airspacei", IDB_AIRSPACEI_ALL },
  { "mountain_top", IDB_MOUNTAIN_TOP_ALL },
  { "bridge", IDB_BRIDGE_ALL },
  { "tunnel", IDB_TUNNEL_ALL },
  { "tower", IDB_TOWER_ALL },
  { "power_plant", IDB_POWER_PLANT_ALL },
  { "airport_marginal", IDB_AIRPORT_MARGINAL_ALL },
  { "outfield_marginal", IDB_OUTFIELD_MARGINAL_ALL },
  { "airport_marginal2", IDB_AIRPORT_MARGINAL2_ALL },
  { "outfield_marginal2", IDB_OUTFIELD_MARGINAL2_ALL },
  { "marginal", IDB_MARGINAL_ALL },
  { "traffic_safe", IDB_TRAFFIC_SAFE_ALL },
  { "traffic_warning", IDB_TRAFFIC_WARNING_ALL },
  { "traffic_alarm", IDB_TRAFFIC_ALARM_ALL },
  { "taskturnpoint", IDB_TASKTURNPOINT_ALL },
  { "obstacle", IDB_OBSTACLE_ALL },
  { "mountain_pass", IDB_MOUNTAIN_PASS_ALL },
  { "weather_station", IDB_WEATHER_STATION_ALL },
  { "thermal_hotspot", IDB_THERMAL_HOTSPOT_ALL },
  { "vor", IDB_VOR_ALL },
  { "ndb", IDB_NDB_ALL },
  { "castle", IDB_CASTLE_ALL },
  { "intersection", IDB_INTERSECTION_ALL },
  { "reporting_point", IDB_REPORTING_POINT_ALL },
  { "pgtakeoff", IDB_PGTAKEOFF_ALL },
  { "pglanding", IDB_PGLANDING_ALL },
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
    if (*endptr != ',')
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
          entry.ultra_icon = ip->ultra_resource_id;
          break;
        }
        ip++;
      }
    }

    // Parse shape field for text display
    entry.shape_field = strtol(p + 1, &endptr, 10) - 1;
    if (*endptr != ',')
      return std::nullopt;

    p = endptr + 1;

    // Parse red component of line / shading colour
    uint8_t red = (uint8_t)strtol(p, &endptr, 10);
    if (*endptr != ',')
      return std::nullopt;

    p = endptr + 1;

    // Parse green component of line / shading colour
    uint8_t green = (uint8_t)strtol(p, &endptr, 10);
    if (*endptr != ',')
      return std::nullopt;

    p = endptr + 1;

    // Parse blue component of line / shading colour
    uint8_t blue = (uint8_t)strtol(p, &endptr, 10);

    p = endptr;

    // Parse pen width of lines
    entry.pen_width = 1;
    if (*p == ',') {
      entry.pen_width = strtoul(p + 1, &endptr, 10);
      if (entry.pen_width < 1)
        entry.pen_width = 1;
      else if (entry.pen_width > 31)
        entry.pen_width = 31;

      p = endptr;
    }

    // Parse range for displaying labels
    entry.label_range = entry.shape_range;
    if (*p == ',') {
      entry.label_range = strtod(p + 1, &endptr) * 1000;
      p = endptr;
    }

    // Parse range for displaying labels with "important" rendering style
    entry.important_label_range = 0;
    if (*p == ',') {
      entry.important_label_range = strtod(p + 1, &endptr) * 1000;
      p = endptr;
    }

    // Handle alpha component
    // If not present at all (i.e. v6.6 or earlier file), default to 100% opaque
    uint8_t alpha = 255;
    if (*p == ',') {
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
