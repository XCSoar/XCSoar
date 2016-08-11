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

#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "Util/StringAPI.hxx"
#include "Util/UTF8.hpp"
#include "Util/StringUtil.hpp"
#include "Util/ScopeExit.hxx"

#ifdef ENABLE_OPENGL
#include "Projection/Projection.hpp"
#include "Screen/OpenGL/Triangulate.hpp"
#endif

#ifdef _UNICODE
#include "Util/ConvertString.hpp"
#endif

#include <algorithm>

#include <tchar.h>

static AllocatedString<TCHAR>
ImportLabel(const char *src)
{
  if (src == nullptr)
    return nullptr;

  src = StripLeft(src);
  if (StringIsEqual(src, "RAILWAY STATION") ||
      StringIsEqual(src, "RAILROAD STATION") ||
      StringIsEqual(src, "UNK"))
    return nullptr;

#ifdef _UNICODE
  return AllocatedString<TCHAR>::Donate(ConvertUTF8ToWide(src));
#else
  if (!ValidateUTF8(src))
    return nullptr;

  return AllocatedString<TCHAR>::Duplicate(src);
#endif
}

/**
 * Returns the minimum number of points for each line of this shape
 * type.  Returns -1 if the shape type is not supported.
 */
gcc_const
static unsigned
GetMinPointsForShapeType(int shapelib_type)
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

XShape::XShape(shapefileObj *shpfile, const GeoPoint &file_center, int i,
               int label_field)
  :label(nullptr)
{
#ifdef ENABLE_OPENGL
  std::fill_n(index_count, THINNING_LEVELS, nullptr);
  std::fill_n(indices, THINNING_LEVELS, nullptr);
#endif

  shapeObj shape;
  msInitShape(&shape);
  AtScopeExit(&shape) { msFreeShape(&shape); };
  msSHPReadShape(shpfile->hSHP, i, &shape);

  bounds = ImportRect(shape.bounds);
  if (!bounds.Check()) {
    /* malformed bounds */
    points = nullptr;
    return;
  }

  type = shape.type;

  num_lines = 0;

  const int min_points = GetMinPointsForShapeType(shape.type);
  if (min_points < 0) {
    /* not supported, leave an empty XShape object */
    points = nullptr;
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
  /* OpenGL: convert GeoPoints to ShapePoints, make them relative to
     the map's boundary center */

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
    for (unsigned j = 0; j < num_points; ++j, ++src) {
#ifdef ENABLE_OPENGL
      const GeoPoint vertex(Angle::Degrees(src->x), Angle::Degrees(src->y));
      const GeoPoint relative = vertex - file_center;

      *p++ = ShapePoint(ShapeScalar(relative.longitude.Native()),
                        ShapeScalar(relative.latitude.Native()));
#else
      *p++ = GeoPoint(Angle::Degrees(src->x),
                      Angle::Degrees(src->y));
#endif
    }
  }

  if (label_field >= 0) {
    const char *src = msDBFReadStringAttribute(shpfile->hDBF, i, label_field);
    label = ImportLabel(src);
  }
}

XShape::~XShape()
{
  delete[] points;
#ifdef ENABLE_OPENGL
  // Note: index_count and indices share one buffer
  for (unsigned i = 0; i < THINNING_LEVELS; i++)
    delete[] index_count[i];
#endif
}

#ifdef ENABLE_OPENGL

bool
XShape::BuildIndices(unsigned thinning_level, ShapeScalar min_distance)
{
  assert(indices[thinning_level] == nullptr);

  uint16_t *idx, *idx_count;
  unsigned num_points = 0;

  for (unsigned i=0; i < num_lines; i++)
    num_points += lines[i];

  if (type == MS_SHAPE_LINE) {
    if (num_points <= 2)
      return false;  // line cannot be simplified, so don't create indices
    index_count[thinning_level] = idx_count =
      new GLushort[num_lines + num_points];
    indices[thinning_level] = idx = idx_count + num_lines;

    const uint16_t *end_l = lines + num_lines;
    const ShapePoint *p = points;
    unsigned i = 0;
    for (const uint16_t *l = lines; l < end_l; l++) {
      assert(*l >= 2);
      const ShapePoint *end_p = p + *l - 1;
      // always add first point
      *idx++ = i;
      p++; i++;
      const uint16_t *after_first_idx = idx;
      // add points if they are not too close to the previous point
      for (; p < end_p; p++, i++)
        if (ManhattanDistance(points[idx[-1]], *p) >= min_distance)
          *idx++ = i;
      // remove points from behind if they are too close to the end point
      while (idx > after_first_idx &&
             ManhattanDistance(points[idx[-1]], *p) < min_distance)
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
      unsigned count = PolygonToTriangles(pt, lines[i], idx + *idx_count,
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
    gcc_unreachable();
  }
}

const uint16_t *
XShape::GetIndices(int thinning_level, ShapeScalar min_distance,
                   const uint16_t *&count) const
{
  if (indices[thinning_level] == nullptr) {
    XShape &deconst = const_cast<XShape &>(*this);
    if (!deconst.BuildIndices(thinning_level, min_distance))
      return nullptr;
  }

  count = index_count[thinning_level];
  return indices[thinning_level];
}

#endif // ENABLE_OPENGL
