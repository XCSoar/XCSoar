/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/StringUtil.hpp"
#include "Util/ConvertString.hpp"
#include "IO/LineReader.hpp"
#include "OS/PathName.hpp"
#include "Operation/Operation.hpp"
#include "Compatibility/path.h"
#include "Asset.hpp"
#include "resource.h"

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
    int resource_id;
} LOOKUP_ICON;

static constexpr LOOKUP_ICON icon_list[] = {
  {"landable", IDB_LANDABLE},
  {"reachable", IDB_REACHABLE},
  {"turnpoint", IDB_TURNPOINT},
  {"small", IDB_SMALL},
  {"cruise", IDB_CRUISE},
  {"town", IDB_TOWN},
  {"mark", IDB_MARK},
  {"terrainwarning", IDB_TERRAINWARNING},
  {"logger", IDB_LOGGER},
  {"loggeroff", IDB_LOGGEROFF},
  {"airport_reachable", IDB_AIRPORT_REACHABLE},
  {"airport_unreachable", IDB_AIRPORT_UNREACHABLE},
  {"outfield_reachable", IDB_OUTFIELD_REACHABLE},
  {"outfield_reachable", IDB_OUTFIELD_UNREACHABLE},
  {"target", IDB_TARGET},
  {"teammate_pos", IDB_TEAMMATE_POS},
  {"airport_unreachable2", IDB_AIRPORT_UNREACHABLE2},
  {"outfield_unreachable2", IDB_OUTFIELD_UNREACHABLE2},
  {"airspacei", IDB_AIRSPACEI},
  {"mountain_top", IDB_MOUNTAIN_TOP},
  {"bridge", IDB_BRIDGE},
  {"tunnel", IDB_TUNNEL},
  {"tower", IDB_TOWER},
  {"power_plant", IDB_POWER_PLANT},
  {"airport_marginal", IDB_AIRPORT_MARGINAL},
  {"outfield_marginal", IDB_OUTFIELD_MARGINAL},
  {"airport_marginal2", IDB_AIRPORT_MARGINAL2},
  {"outfield_marginal2", IDB_OUTFIELD_MARGINAL2},
  {"marginal", IDB_MARGINAL},
  {"traffic_safe", IDB_TRAFFIC_SAFE},
  {"traffic_warning", IDB_TRAFFIC_WARNING},
  {"traffic_alarm", IDB_TRAFFIC_ALARM},
  {"taskturnpoint", IDB_TASKTURNPOINT},
  {"obstacle", IDB_OBSTACLE},
  {"mountain_pass", IDB_MOUNTAIN_PASS},
  {"weather_station", IDB_WEATHER_STATION},
  {"thermal_hotspot", IDB_THERMAL_HOTSPOT},
  { NULL, 0 }
};

unsigned
TopographyStore::ScanVisibility(const WindowProjection &m_projection,
                              unsigned max_update)
{
  // check if any needs to have cache updates because wasnt
  // visible previously when bounds moved

  // we will make sure we update at least one cache per call
  // to make sure eventually everything gets refreshed
  unsigned num_updated = 0;
  for (auto it = files.begin(), end = files.end(); it != end; ++it) {
    if ((*it)->Update(m_projection)) {
      ++num_updated;
      if (num_updated >= max_update)
        break;
    }
  }

  return num_updated;
}

void
TopographyStore::LoadAll()
{
  for (const auto &i : files)
    i->LoadAll();
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
  if (directory != NULL) {
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
  while (!files.full() && (line = reader.ReadLine()) != NULL) {
    // Line format: filename,range,icon,field,r,g,b,pen_width,label_range,important_range

    // Ignore comments (lines starting with *) and empty lines
    if (StringIsEmpty(line) || line[0] == '*')
      continue;

    // Find first comma to extract shape filename
    char *p = strchr(line, ',');
    if (p == NULL || p == line)
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
    fixed shape_range = fixed(strtod(p + 1, &p)) * 1000;
    if (*p != _T(','))
      continue;

    // Extract shape icon name
    char icon_name[23];
    char *start = p + 1;
    p = strchr(start, ',');
    // Null-terminate the line string at the next comma for strncpy() call
    *p = 0;
    strncpy(icon_name, start, 22);
    unsigned icon = 0;

    if (strlen(icon_name) > 0) {
      const LOOKUP_ICON *ip = icon_list;
      while (ip->name != NULL) {
        if (StringIsEqual(ip->name, icon_name)) {
          icon = ip->resource_id;
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
    int pen_width=1;
    if (*p == _T(',')) {
      pen_width = strtol(p + 1, &p, 10);
      if (pen_width<0)
        pen_width=1;
      else if (pen_width>31)
        pen_width=31;
    }

    // Parse range for displaying labels
    fixed label_range = shape_range;
    if (*p == _T(','))
      label_range = fixed(strtod(p + 1, &p)) * 1000;

    // Parse range for displaying labels with "important" rendering style
    fixed labelImportantRange = fixed(0);
    if (*p == _T(','))
      labelImportantRange = fixed(strtod(p + 1, &p)) * 1000;

    // Create TopographyFile instance from parsed line
    TopographyFile *file = new TopographyFile(zdir, shape_filename,
                                              shape_range, label_range,
                                              labelImportantRange,
                                              Color(red, green, blue),
                                              shape_field, icon,
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
  for (auto it = files.begin(), end = files.end(); it != end; ++it)
    delete *it;

  files.clear();
}
