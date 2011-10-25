/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "StringUtil.hpp"
#include "IO/LineReader.hpp"
#include "OS/PathName.hpp"
#include "Operation.hpp"
#include "Compatibility/path.h"
#include "Asset.hpp"

#include <stdint.h>
#include <windef.h> // for MAX_PATH

static bool
IsHugeTopographyFile(const char *name)
{
  return strcmp(name, "village_point") == 0 ||
    strcmp(name, "citysmall_point") == 0 ||
    strcmp(name, "roadsmall_point") == 0 ||
    strcmp(name, "roadsmall_line") == 0;
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
  for (unsigned i = 0; i < files.size(); ++i) {
    if (files[i]->Update(m_projection)) {
      ++num_updated;
      if (num_updated >= max_update)
        break;
    }
  }

  return num_updated;
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

  char shape_filename[MAX_PATH];
  if (directory != NULL) {
    strcpy(shape_filename, NarrowPathName(directory));
    strcat(shape_filename, DIR_SEPARATOR_S);
  } else
    shape_filename[0] = 0;

  char *shape_filename_end = shape_filename + strlen(shape_filename);

  long filesize = std::max(reader.size(), 1l);

  operation.SetProgressRange(100);

  char *line;
  while (!files.full() && (line = reader.read()) != NULL) {
    // Look For Comment
    if (string_is_empty(line) || line[0] == '*')
      continue;

    // filename,range,icon,field

    // File name
    char *p = strchr(line, ',');
    if (p == NULL || p == line)
      continue;

    if (HasLittleMemory()) {
      /* hard-coded blacklist for huge files on PPC2000; those
         devices usually have very little memory */

      *p = 0;

      if (IsHugeTopographyFile(line))
        continue;
    }

    memcpy(shape_filename_end, line, p - line);

    strcpy(shape_filename_end + (p - line), ".shp");

    // Shape range
    double shape_range = strtod(p + 1, &p);
    if (*p != _T(','))
      continue;

    // Shape icon
    long shape_icon = strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Shape field for text display
    // sjt 02NOV05 - field parameter enabled
    long shape_field = strtol(p + 1, &p, 10) - 1;
    if (*p != _T(','))
      continue;

    // Red component of line / shading colour
    uint8_t red = (uint8_t)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Green component of line / shading colour
    uint8_t green = (uint8_t)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Blue component of line / shading colour
    uint8_t blue = (uint8_t)strtol(p + 1, &p, 10);

    // Pen width of lines
    int pen_width=1;
    if (*p == _T(',')) {
      pen_width = strtol(p + 1, &p, 10);
      if (pen_width<0)
        pen_width=1;
      else if (pen_width>31)
        pen_width=31;
    }

    // range for displaying labels
    double label_range = shape_range;
    if (*p == _T(','))
      label_range = strtod(p + 1, &p);

    // range for displaying labels with "important" rendering style
    double labelImportantRange = 0;
    if (*p == _T(','))
      labelImportantRange = strtod(p + 1, &p);

    files.append(new TopographyFile(zdir, shape_filename,
                                  fixed(shape_range) * 1000,
                                  fixed(label_range) * 1000,
                                  fixed(labelImportantRange) * 1000,
                                  Color(red, green, blue),
                                  shape_field, shape_icon,
                                  pen_width));

    operation.SetProgressPosition((reader.tell() * 100) / filesize);
  }
}

void
TopographyStore::Reset()
{
  for (unsigned i = 0; i < files.size(); ++i)
    delete files[i];

  files.clear();
}
