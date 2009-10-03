#include "PolygonInterior.hpp"

///////////////////////////////////////////////////

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
isLeft( const GEOPOINT &P0, const GEOPOINT &P1, const GEOPOINT &P2 )
{
    return ( (P1.Longitude - P0.Longitude) * (P2.Latitude - P0.Latitude)
            - (P2.Longitude - P0.Longitude) * (P1.Latitude - P0.Latitude) );
}
//===================================================================

// PolygonInterior(): winding number interior test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  true if P is inside V

bool
PolygonInterior( const GEOPOINT &P, const std::vector<SearchPoint>& V)
{
  int n = V.size()-1;
  if (n<2) {
    return false;
  }

  int    wn = 0;    // the winding number counter

  // loop through all edges of the polygon
  for (int i=0; i<n; i++) {   // edge from V[i] to V[i+1]
    if (V[i].getLocation().Latitude <= P.Latitude) {         // start y <= P.Latitude
      if (V[i+1].getLocation().Latitude > P.Latitude)      // an upward crossing
        if (isLeft( V[i].getLocation(), V[i+1].getLocation(), P) > 0)  // P left of edge
          ++wn;            // have a valid up intersect
    }
    else {                       // start y > P.Latitude (no test needed)
      if (V[i+1].getLocation().Latitude <= P.Latitude)     // a downward crossing
        if (isLeft( V[i].getLocation(), V[i+1].getLocation(), P) < 0)  // P right of edge
          --wn;            // have a valid down intersect
    }
  }
  return wn != 0;
}
