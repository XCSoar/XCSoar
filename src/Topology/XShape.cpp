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

#include "Topology/XShape.hpp"
#include "Units.hpp"
#include "shapelib/map.h"

#include <algorithm>
#include <tchar.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#ifdef _UNICODE
#include <windows.h>
#endif

static TCHAR *
import_label(const char *src)
{
  if (src == NULL || strcmp(src, "UNK") == 0 ||
      strcmp(src, "RAILWAY STATION") == 0 ||
      strcmp(src, "RAILROAD STATION") == 0)
    return NULL;

  if (ispunct(src[0])) {
    fixed value(strtod(src + 1, NULL));
    value = Units::ToUserUnit(value, Units::AltitudeUnit);

    TCHAR buffer[32];
    if (value > fixed(999))
      _stprintf(buffer, _T("%.1f"), (double)(value / 1000));
    else
      _stprintf(buffer, _T("%d"), (int)value);

    return _tcsdup(buffer);
  }

#ifdef _UNICODE
  size_t length = strlen(src);
  TCHAR *dest = new TCHAR[length + 1];
  if (::MultiByteToWideChar(CP_UTF8, 0, src, -1, dest, length + 1) <= 0) {
    delete[] dest;
    return NULL;
  }

  return dest;
#else
  return strdup(src);
#endif
}

XShape::XShape(shapefileObj *shpfile, int i, int label_field)
  :label(NULL)
{
  shapeObj shape;
  msInitShape(&shape);
  msSHPReadShape(shpfile->hSHP, i, &shape);

  bounds.west = Angle::degrees(fixed(shape.bounds.minx));
  bounds.south = Angle::degrees(fixed(shape.bounds.miny));
  bounds.east = Angle::degrees(fixed(shape.bounds.maxx));
  bounds.north = Angle::degrees(fixed(shape.bounds.maxy));

  type = shape.type;

  num_lines = std::min((unsigned)shape.numlines, (unsigned)MAX_LINES);

  unsigned num_points = 0;
  for (unsigned l = 0; l < num_lines; ++l) {
    lines[l] = std::min(shape.line[l].numpoints, 16384);
    num_points += lines[l];
  }

  /* convert all points of all lines to GeoPoints */

  points = new GeoPoint[num_points];

  GeoPoint *p = points;
  for (unsigned l = 0; l < num_lines; ++l) {
    const pointObj *src = shape.line[l].point;
    num_points = lines[l];
    for (unsigned j = 0; j < num_points; ++j, ++src)
      *p++ = GeoPoint(Angle::degrees(fixed(src->x)),
                      Angle::degrees(fixed(src->y)));
  }

  if (label_field >= 0) {
    const char *src = msDBFReadStringAttribute(shpfile->hDBF, i, label_field);
    label = import_label(src);
  }

  msFreeShape(&shape);
}

XShape::~XShape()
{
  free(label);
  delete[] points;
}
