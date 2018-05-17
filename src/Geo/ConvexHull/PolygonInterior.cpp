/* Copyright_License {

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

#include "PolygonInterior.hpp"
#include "Math/Line2D.hpp"

static constexpr Point2D<double>
GeoTo2D(GeoPoint p)
{
  return {p.longitude.Native(), p.latitude.Native()};
}

// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

//    a Point is defined by its coordinates {int x, y;}
//===================================================================

// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2 on the line
//            <0 for P2 right of the line
//    See: the January 2001 Algorithm "Area of 2D and 3D Triangles and Polygons"
inline static double
isLeft( const GeoPoint &P0, const GeoPoint &P1, const GeoPoint &P2 )
{
  return Line2D<Point2D<double>>(GeoTo2D(P0), GeoTo2D(P1)).LocatePoint(GeoTo2D(P2));
}

inline static int
isLeft( const FlatGeoPoint &P0, const FlatGeoPoint &P1, const FlatGeoPoint &P2 )
{
  return Line2D<FlatGeoPoint>(P0, P1).LocatePoint(P2);
}

//===================================================================

// PolygonInterior(): winding number interior test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  true if P is inside V

bool
PolygonInterior(const GeoPoint &P,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end)
{
  if (std::distance(begin, end) < 3)
    return false;

  int    wn = 0;    // the winding number counter

  // loop through all edges of the polygon
  for (auto i = begin, next = std::next(i); next != end;
       i = next, next = std::next(i)) {
    // edge from current to next
    if (i->GetLocation().latitude <= P.latitude) {
      // start y <= P.latitude

      if (next->GetLocation().latitude > P.latitude)
        // an upward crossing
        if (isLeft(i->GetLocation(), next->GetLocation(), P) > 0)
          // P left of edge
          // have a valid up intersect
          ++wn;
    } else {
      // start y > P.latitude (no test needed)

      if (next->GetLocation().latitude <= P.latitude)
        // a downward crossing
        if (isLeft(i->GetLocation(), next->GetLocation(), P) < 0)
          // P right of edge
          // have a valid down intersect
          --wn;
    }
  }
  return wn != 0;
}


bool
PolygonInterior(const FlatGeoPoint &P,
                SearchPointVector::const_iterator begin,
                SearchPointVector::const_iterator end)
{
  if (std::distance(begin, end) < 3)
    return false;

  int    wn = 0;    // the winding number counter

  // loop through all edges of the polygon
  for (auto i = begin, next = std::next(i); next != end;
       i = next, next = std::next(i)) {
    // edge from current to next
    if (i->GetFlatLocation().y <= P.y) {
      // start y <= P.y
      if (next->GetFlatLocation().y > P.y)
        // an upward crossing
        if (isLeft(i->GetFlatLocation(), next->GetFlatLocation(), P) > 0)
          // P left of edge
          // have a valid up intersect
          ++wn;
    } else {
      // start y > P.y (no test needed)

      if (next->GetFlatLocation().y <= P.y)
        // a downward crossing
        if (isLeft(i->GetFlatLocation(), next->GetFlatLocation(), P) < 0)
          // P right of edge
          // have a valid down intersect
          --wn;
    }
  }
  return wn != 0;
}


