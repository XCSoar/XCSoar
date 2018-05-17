/*
Copyright_License {

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

#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyFile.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringCompare.hxx"
#include "Util/ConvertString.hpp"
#include "IO/LineReader.hpp"
#include "Operation/Operation.hpp"
#include "Compatibility/path.h"
#include "Asset.hpp"
#include "Resources.hpp"

#include <stdint.h>
#include <windef.h> // for MAX_PATH

static bool
IsHugeTopographyFile(const char *name)
{
  return StringIsEqual(name, "village_point") ||
    StringIsEqual(name, "citysmall_point") ||
    StringIsEqual(name, "roadsmall_point") ||
    StringIsEqual(name, "roadsmall_line");
}

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

double
TopographyStore::GetNextScaleThreshold(double map_scale) const
{
  double result(-1);
  for (auto *file : files) {
    double threshold = file->GetNextScaleThreshold(map_scale);
    if (threshold > result)
      result = threshold;
  }

  return result;
}

unsigned
TopographyStore::ScanVisibility(const WindowProjection &m_projection,
                              unsigned max_update)
{
  // check if any needs to have cache updates because wasnt
  // visible previously when bounds moved

  // we will make sure we update at least one cache per call
  // to make sure eventually everything gets refreshed
  unsigned num_updated = 0;
  for (auto *file : files) {
    if (file->Update(m_projection)) {
      ++num_updated;
      if (num_updated >= max_update)
        break;
    }
  }

  serial += num_updated;
  return num_updated;
}

void
TopographyStore::LoadAll()
{
  for (const auto &i : files) {
    TopographyFile &file = *i;
    file.LoadAll();
  }
}

TopographyStore::~TopographyStore()
{
  Reset();
}

void
TopographyStore::Load(OperationEnvironment &operation, NLineReader &reader,
                      const TCHAR *directory, struct zzip_dir *zdir)
{
  Reset();

  // Create buffer for the shape filenames
  // (shape_filename will be modified with the shape_filename_end pointer)
  char shape_filename[MAX_PATH];
  if (directory != nullptr) {
    const WideToACPConverter narrow_directory(directory);
    strcpy(shape_filename, narrow_directory);
    strcat(shape_filename, DIR_SEPARATOR_S);
  } else
    shape_filename[0] = 0;

  char *shape_filename_end = shape_filename + strlen(shape_filename);

  // Read file size to have a rough progress estimate for the progress bar
  const long filesize = std::max(reader.GetSize(), 1l);

  // Set progress bar to 100 steps
  operation.SetProgressRange(100);

  // Iterate through shape files in the "topology.tpl" file until
  // end or max. file number reached
  char *line;
  while (!files.full() && (line = reader.ReadLine()) != nullptr) {
    // .tpl Line format: filename,range,icon,field,r,g,b,pen_width,label_range,important_range,alpha

    // Ignore comments (lines starting with *) and empty lines
    if (StringIsEmpty(line) || line[0] == '*')
      continue;

    // Find first comma to extract shape filename
    char *p = strchr(line, ',');
    if (p == nullptr || p == line)
      // If no comma was found -> ignore this line/shapefile
      continue;

    if (HasLittleMemory()) {
      /* hard-coded blacklist for huge files on PPC2000; those
         devices usually have very little memory */

      // Null-terminate the line string after the first comma
      // for strcmp() calls in IsHugeTopographyFile() function
      *p = 0;

      // Skip large topography files
      if (IsHugeTopographyFile(line))
        continue;
    }

    // Extract filename and append it to the shape_filename buffer
    memcpy(shape_filename_end, line, p - line);
    // Append ".shp" file extension to the shape_filename buffer
    strcpy(shape_filename_end + (p - line), ".shp");

    // Parse shape range
    auto shape_range = strtod(p + 1, &p) * 1000;
    if (*p != _T(','))
      continue;

    // Extract shape icon name
    char icon_name[23];
    char *start = p + 1;
    p = strchr(start, ',');
    // Null-terminate the line string at the next comma for strncpy() call
    *p = 0;
    strncpy(icon_name, start, 22);
    ResourceId icon = ResourceId::Null(), big_icon = ResourceId::Null();

    if (strlen(icon_name) > 0) {
      const LOOKUP_ICON *ip = icon_list;
      while (ip->name != nullptr) {
        if (StringIsEqual(ip->name, icon_name)) {
          icon = ip->resource_id;
          big_icon = ip->big_resource_id;
          break;
        }
        ip++;
      }
    }

    // Parse shape field for text display
    long shape_field = strtol(p + 1, &p, 10) - 1;
    if (*p != _T(','))
      continue;

    // Parse red component of line / shading colour
    uint8_t red = (uint8_t)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Parse green component of line / shading colour
    uint8_t green = (uint8_t)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Parse blue component of line / shading colour
    uint8_t blue = (uint8_t)strtol(p + 1, &p, 10);

    // Parse pen width of lines
    unsigned pen_width = 1;
    if (*p == _T(',')) {
      pen_width = strtoul(p + 1, &p, 10);
      if (pen_width < 1)
        pen_width = 1;
      else if (pen_width>31)
        pen_width=31;
    }

    // Parse range for displaying labels
    auto label_range = shape_range;
    if (*p == _T(','))
      label_range = strtod(p + 1, &p) * 1000;

    // Parse range for displaying labels with "important" rendering style
    double labelImportantRange = 0;
    if (*p == _T(','))
      labelImportantRange = strtod(p + 1, &p) * 1000;

    // Handle alpha component
    // If not present at all (i.e. v6.6 or earlier file), default to 100% opaque
    uint8_t alpha = 255;
    if (*p == _T(',')) {
      // An alpha component of shading colour is present (v6.7 or later file).
      alpha = (uint8_t)strtol(p + 1, &p, 10);
      // Ignore a totally transparent file!
      if (alpha == 0)
        continue;
#ifndef ENABLE_OPENGL
      // Without OpenGL ignore anything but 100% opaque
      if (alpha != 255)
        continue;
#endif
    }

    // Create TopographyFile instance from parsed line
    TopographyFile *file = new TopographyFile(zdir, shape_filename,
                                              shape_range, label_range,
                                              labelImportantRange,
#ifdef ENABLE_OPENGL
                                              Color(red, green, blue, alpha),
#else
                                              Color(red, green, blue),
#endif
                                              shape_field, icon, big_icon,
                                              pen_width);
    if (file->IsEmpty())
      // If the shape file could not be read -> skip this line/file
      delete file;
    else
      // .. otherwise append it to our list of shape files
      files.append(file);

    // Update progress bar
    operation.SetProgressPosition((reader.Tell() * 100) / filesize);
  }
}

void
TopographyStore::Reset()
{
  for (auto *file : files)
    delete file;

  files.clear();
}
