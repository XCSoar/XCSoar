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
#include "Compatibility/path.h"

#include <windef.h> // for MAX_PATH

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
    if (files[i]->updateCache(m_projection)) {
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
TopographyStore::Load(NLineReader &reader, StatusCallback callback,
                      const TCHAR *Directory, struct zzip_dir *zdir)
{
  Reset();

  double ShapeRange;
  long ShapeIcon;
  long ShapeField;
  char ShapeFilename[MAX_PATH];

  if (Directory != NULL) {
    strcpy(ShapeFilename, NarrowPathName(Directory));
    strcat(ShapeFilename, DIR_SEPARATOR_S);
  } else
    ShapeFilename[0] = 0;

  char *ShapeFilenameEnd = ShapeFilename + strlen(ShapeFilename);

  long filesize = std::max(reader.size(), 1l);

  char *line;
  while (!files.full() && (line = reader.read()) != NULL) {
    // Look For Comment
    if (string_is_empty(line) || line[0] == '*')
      continue;


    BYTE red, green, blue;
    // filename,range,icon,field

    // File name
    char *p = strchr(line, ',');
    if (p == NULL || p == line)
      continue;

    memcpy(ShapeFilenameEnd, line, p - line);
    strcpy(ShapeFilenameEnd + (p - line), ".shp");

    // Shape range
    ShapeRange = strtod(p + 1, &p);
    if (*p != _T(','))
      continue;

    // Shape icon
    ShapeIcon = strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Shape field for text display
    // sjt 02NOV05 - field parameter enabled
    ShapeField = strtol(p + 1, &p, 10) - 1;
    if (*p != _T(','))
      continue;

    // Red component of line / shading colour
    red = (BYTE)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Green component of line / shading colour
    green = (BYTE)strtol(p + 1, &p, 10);
    if (*p != _T(','))
      continue;

    // Blue component of line / shading colour
    blue = (BYTE)strtol(p + 1, &p, 10);

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
    double labelRange = ShapeRange;
    if (*p == _T(','))
      labelRange = strtod(p + 1, &p);

    // range for displaying labels with "important" rendering style
    double labelImportantRange = 0;
    if (*p == _T(','))
      labelImportantRange = strtod(p + 1, &p);

    files.append(new TopographyFile(zdir, ShapeFilename,
                                  fixed(ShapeRange) * 1000,
                                  fixed(labelRange) * 1000,
                                  fixed(labelImportantRange) * 1000,
                                  Color(red, green, blue),
                                  ShapeField, ShapeIcon,
                                  pen_width));

    if (callback != NULL)
      callback((reader.tell() * 100) / filesize);
  }
}

void
TopographyStore::Reset()
{
  for (unsigned i = 0; i < files.size(); ++i)
    delete files[i];

  files.clear();
}
