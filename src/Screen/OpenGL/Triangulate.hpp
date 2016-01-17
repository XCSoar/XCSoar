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

#ifndef XCSOAR_SCREEN_OPENGL_TRIANGULATE_HPP
#define XCSOAR_SCREEN_OPENGL_TRIANGULATE_HPP

#include "System.hpp"

struct BulkPixelPoint;
struct FloatPoint2D;
template<class T> class AllocatedArray;

/**
 * cutting ears - simple algorithm, no support for holes
 * Optionally removes all points from a polygon that are too close together.
 *
 * @param points polygon coordinates
 * @param num_points number of polygon vertices
 * @param triangles triangle indices, size: 3*(num_points-2)
 * @param min_distance minimum distance a point should have from its neighbours
 *
 * @return Returns the number of triangle indices. Possible values:
 *         0: failure,
 *         3 to 3*(num_points-2): success
 */
unsigned
PolygonToTriangles(const BulkPixelPoint *points, unsigned num_points,
                   AllocatedArray<GLushort> &triangles,
                   unsigned min_distance=1);
unsigned
PolygonToTriangles(const FloatPoint2D *points, unsigned num_points,
                   GLushort *triangles, float min_distance=1);

/**
 * Pack triangle indices into a triangle strip.
 * Empty triangles are inserted to connect individual strips. Thus we always
 * get one "degenerated" triangle strip. The degenerated triangles should
 * be discarded pretty early in the rendering pipeline. This saves a lot of
 * OpenGL API calls.
 * The triangle buffer must hold at least:
 *   3*(triangle_count-2) + 2*(polygon_count-1) indices.
 *
 * @param triangles triangle indicies, which will be overwriten with the strip
 * @param index_count number of triangle indices. (triangle_count*3)
 * @param vertex_count number of vertices used: max(triangles[i])+1
 * @param polygon_count number of unconnected polygons
 *
 * @return number of indices in the triangle strip
 */
unsigned
TriangleToStrip(GLushort *triangles, unsigned index_count,
                unsigned vertex_count, unsigned polygon_count=1);

/**
 * Create a triangle strip representing a thick line.
 *
 * @param points line coordinates
 * @param num_points number of line points
 * @param strip buffer for triangle vertices
 * @param line_width width of line in pixels
 * @param loop true if line is a closed loop
 * @param tcap add a triangle at the beginning and end of the line
 *
 * @return Returns the number of triangle coordinates or 0 for failure
 */
unsigned
LineToTriangles(const BulkPixelPoint *points, unsigned num_points,
                AllocatedArray<BulkPixelPoint> &strip,
                unsigned line_width, bool loop=false, bool tcap=false);

#endif
