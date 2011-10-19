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

#include "Topography/XShape.hpp"
#include "Util/UTF8.hpp"
#include "Units/Units.hpp"
#include "shapelib/mapserver.h"
#ifdef ENABLE_OPENGL
#include "Projection/Projection.hpp"
#include "Math/Earth.hpp"
#include "Screen/OpenGL/Triangulate.hpp"
#endif

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
    value = Units::ToUserAltitude(value);

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
  if (!ValidateUTF8(src))
    return NULL;

  return strdup(src);
#endif
}

/**
 * Returns the minimum number of points for each line of this shape
 * type.  Returns -1 if the shape type is not supported.
 */
gcc_const
static unsigned
min_points_for_type(int shapelib_type)
{
  switch (shapelib_type) {
  case MS_SHAPE_POINT:
    return 1;

  case MS_SHAPE_LINE:
    return 2;

  case MS_SHAPE_POLYGON:
    return 3;

  default:
    /* not supported */
    return -1;
  }
}

XShape::XShape(shapefileObj *shpfile, int i, int label_field)
  :label(NULL)
{
#ifdef ENABLE_OPENGL
  for (unsigned l=0; l < THINNING_LEVELS; l++)
    index_count[l] = indices[l] = NULL;
#endif

  shapeObj shape;
  msInitShape(&shape);
  msSHPReadShape(shpfile->hSHP, i, &shape);

  bounds.west = Angle::Degrees(fixed(shape.bounds.minx));
  bounds.south = Angle::Degrees(fixed(shape.bounds.miny));
  bounds.east = Angle::Degrees(fixed(shape.bounds.maxx));
  bounds.north = Angle::Degrees(fixed(shape.bounds.maxy));
#ifdef ENABLE_OPENGL
  center = bounds.center();
#endif

  type = shape.type;

  num_lines = 0;

  const int min_points = min_points_for_type(shape.type);
  if (min_points < 0) {
    /* not supported, leave an empty XShape object */
    points = NULL;
    msFreeShape(&shape);
    return;
  }

  const unsigned input_lines = std::min((unsigned)shape.numlines,
                                        (unsigned)MAX_LINES);
  unsigned num_points = 0;
  for (unsigned l = 0; l < input_lines; ++l) {
    if (shape.line[l].numpoints < min_points)
      /* malformed shape */
      continue;

    lines[num_lines] = std::min(shape.line[l].numpoints, 16384);
    num_points += lines[num_lines];
    ++num_lines;
  }

#ifdef ENABLE_OPENGL
  /* OpenGL:
   * Convert all points of all lines to ShapePoints, using a projection
   * that assumes the center of the screen is also the center of the shape.
   * Resolution is set to 1m per pixel. This enables us to use a simple matrix
   * multiplication to draw the shape.
   * This approximation should work well with shapes of limited size
   * (<< 400km). Perceivable distortion will only happen, when the latitude of
   * the actual center of the screen is far away from the latitude of the
   * center of the shape and the shape has a big vertical size.
   */

  points = new ShapePoint[num_points];
  ShapePoint *p = points;
#else // !ENABLE_OPENGL
  /* convert all points of all lines to GeoPoints */

  points = new GeoPoint[num_points];
  GeoPoint *p = points;
#endif
  for (unsigned l = 0; l < num_lines; ++l) {
    const pointObj *src = shape.line[l].point;
    num_points = lines[l];
    for (unsigned j = 0; j < num_points; ++j, ++src)
#ifdef ENABLE_OPENGL
      *p++ = geo_to_shape(GeoPoint(Angle::Degrees(fixed(src->x)),
                                   Angle::Degrees(fixed(src->y))));
#else
      *p++ = GeoPoint(Angle::Degrees(fixed(src->x)),
                      Angle::Degrees(fixed(src->y)));
#endif
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
#ifdef ENABLE_OPENGL
  // Note: index_count and indices share one buffer
  for (int i=0; i < THINNING_LEVELS; i++)
    delete[] index_count[i];
#endif
}

#ifdef ENABLE_OPENGL

bool
XShape::BuildIndices(unsigned thinning_level, unsigned min_distance)
{
  assert(indices[thinning_level] == NULL);

  unsigned short *idx, *idx_count;
  unsigned num_points = 0;

  for (unsigned i=0; i < num_lines; i++)
    num_points += lines[i];

  if (type == MS_SHAPE_LINE) {
    if (num_points <= 2)
      return false;  // line cannot be simplified, so don't create indices
    index_count[thinning_level] = idx_count =
      new GLushort[num_lines + num_points];
    indices[thinning_level] = idx = idx_count + num_lines;

    const unsigned short *end_l = lines + num_lines;
    const ShapePoint *p = points;
    unsigned i = 0;
    for (const unsigned short *l = lines; l < end_l; l++) {
      assert(*l >= 2);
      const ShapePoint *end_p = p + *l - 1;
      // always add first point
      *idx++ = i;
      p++; i++;
      const unsigned short *after_first_idx = idx;
      // add points if they are not too close to the previous point
      for (; p < end_p; p++, i++)
        if (manhattan_distance(points[idx[-1]], *p) >= min_distance)
          *idx++ = i;
      // remove points from behind if they are too close to the end point
      while (idx > after_first_idx &&
             manhattan_distance(points[idx[-1]], *p) < min_distance)
        idx--;
      // always add last point
      *idx++ = i;
      p++; i++;
      *idx_count++ = idx - after_first_idx + 1;
    }
    // TODO: free memory saved by thinning (use malloc/realloc or some class?)
    return true;
  } else if (type == MS_SHAPE_POLYGON) {
    index_count[thinning_level] = idx_count =
      new GLushort[1 + 3*(num_points-2) + 2*(num_lines-1)];
    indices[thinning_level] = idx = idx_count + 1;

    *idx_count = 0;
    const ShapePoint *pt = points;
    for (unsigned i=0; i < num_lines; i++) {
      unsigned count = PolygonToTriangle(pt, lines[i], idx + *idx_count,
                                         min_distance);
      if (i > 0) {
        const GLushort offset = pt - points;
        const unsigned max_idx_count = *idx_count + count;
        for (unsigned j=*idx_count; j < max_idx_count; j++)
          idx[j] += offset;
      }
      *idx_count += count;
      pt += lines[i];
    }
    *idx_count = TriangleToStrip(idx, *idx_count, num_points, num_lines);
    // TODO: free memory saved by thinning (use malloc/realloc or some class?)
    return true;
  } else {
    assert(false);
    return false;
  }
}

const unsigned short *
XShape::get_indices(int thinning_level, unsigned min_distance,
                    const unsigned short *&count) const
{
  if (indices[thinning_level] == NULL) {
    XShape &deconst = const_cast<XShape &>(*this);
    if (!deconst.BuildIndices(thinning_level, min_distance))
      return NULL;
  }

  count = index_count[thinning_level];
  return indices[thinning_level];
}

ShapePoint
XShape::geo_to_shape(const GeoPoint &origin, const GeoPoint &point) const
{
  const GeoPoint d = point-origin;

  ShapePoint pt;
  pt.x = (ShapeScalar)fast_mult(point.latitude.fastcosine(),
                                fast_mult(d.longitude.Radians(),
                                          fixed_earth_r, 12), 16);
  pt.y = (ShapeScalar)-fast_mult(d.latitude.Radians(), fixed_earth_r, 12);
  return pt;
}

#endif // ENABLE_OPENGL
