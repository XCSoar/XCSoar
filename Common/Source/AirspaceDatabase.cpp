/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "AirspaceDatabase.hpp"
#include "Airspace.h"
#include "GeoPoint.hpp"
#include "Math/Earth.hpp"
#include "Math/Pressure.h"
#include "Math/Screen.hpp"
#include "Math/Units.h"
#include "SettingsComputer.hpp"
#include "MapWindowProjection.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * Constructor of the AirspaceDatabase
 */
AirspaceDatabase::AirspaceDatabase()
  :AirspacePoint(NULL), NumberOfAirspacePoints(0),
   AirspaceArea(NULL), NumberOfAirspaceAreas(0),
   AirspaceCircle(NULL), NumberOfAirspaceCircles(0),
   qnh(0.0)
{
}

/**
 * Clear the AirspaceDatabase
 */
void
AirspaceDatabase::Clear()
{
  NumberOfAirspacePoints = 0;
  NumberOfAirspaceAreas = 0;
  NumberOfAirspaceCircles = 0;

  if (AirspaceArea != NULL)   {
    LocalFree((HLOCAL)AirspaceArea);
    AirspaceArea = NULL;
  }

  if (AirspacePoint != NULL)  {
    LocalFree((HLOCAL)AirspacePoint);
    AirspacePoint = NULL;
  }

  if (AirspaceCircle != NULL) {
    LocalFree((HLOCAL)AirspaceCircle);
    AirspaceCircle = NULL;
  }
}

bool
AirspaceDatabase::GrowPoints(unsigned n)
{
  if (n == 0)
    return true;

  size_t size = (NumberOfAirspacePoints + n) * sizeof(AirspacePoint[0]);
  AIRSPACE_POINT *new_points = AirspacePoint == NULL
    ? (AIRSPACE_POINT *)LocalAlloc(LMEM_FIXED, size)
    : (AIRSPACE_POINT *)LocalReAlloc(AirspacePoint, LMEM_FIXED, size);
  if (new_points == NULL)
    return false;

  AirspacePoint = new_points;
  return true;
}

bool
AirspaceDatabase::GrowAreas(unsigned n)
{
  if (n == 0)
    return true;

  size_t size = (NumberOfAirspaceAreas + n) * sizeof(AirspaceArea[0]);
  AIRSPACE_AREA *new_areas = AirspaceArea == NULL
    ? (AIRSPACE_AREA *)LocalAlloc(LMEM_FIXED, size)
    : (AIRSPACE_AREA *)LocalReAlloc(AirspaceArea, LMEM_FIXED, size);
  if (new_areas == NULL)
    return false;

  AirspaceArea = new_areas;
  return true;
}

bool
AirspaceDatabase::GrowCircles(unsigned n)
{
  if (n == 0)
    return true;

  size_t size = (NumberOfAirspaceCircles + n) * sizeof(AirspaceCircle[0]);
  AIRSPACE_CIRCLE *new_circles = AirspaceCircle == NULL
    ? (AIRSPACE_CIRCLE *)LocalAlloc(LMEM_FIXED, size)
    : (AIRSPACE_CIRCLE *)LocalReAlloc(AirspaceCircle, LMEM_FIXED, size);
  if (new_circles == NULL)
    return false;

  AirspaceCircle = new_circles;
  return true;
}

/**
 * Compare function for AirspaceAreas (sorts by priority)
 * @param elem1 First AirspaceArea
 * @param elem2 Second AirspaceArea
 */
static int _cdecl SortAirspaceAreaCompare(const void *elem1, const void *elem2 )
{
  if (AirspacePriority[((AIRSPACE_AREA *)elem1)->Type] >
      AirspacePriority[((AIRSPACE_AREA *)elem2)->Type])
    return (-1);
  if (AirspacePriority[((AIRSPACE_AREA *)elem1)->Type] <
      AirspacePriority[((AIRSPACE_AREA *)elem2)->Type])
    return (+1);

  // otherwise sort on height?
  return (0);
}

/**
 * Compare function for AirspaceCircles (sorts by priority)
 * @param elem1 First AirspaceCircle
 * @param elem2 Second AirspaceCircle
 */
static int _cdecl SortAirspaceCircleCompare(const void *elem1, const void *elem2 )
{
  if (AirspacePriority[((AIRSPACE_CIRCLE *)elem1)->Type] >
      AirspacePriority[((AIRSPACE_CIRCLE *)elem2)->Type])
    return (-1);
  if (AirspacePriority[((AIRSPACE_CIRCLE *)elem1)->Type] <
      AirspacePriority[((AIRSPACE_CIRCLE *)elem2)->Type])
    return (+1);

  // otherwise sort on height?
  return (0);
}

void
AirspaceDatabase::Sort()
{
  qsort(AirspaceArea, NumberOfAirspaceAreas, sizeof(AIRSPACE_AREA),
        SortAirspaceAreaCompare);

  qsort(AirspaceCircle, NumberOfAirspaceCircles, sizeof(AIRSPACE_CIRCLE),
        SortAirspaceCircleCompare);
}

static void
SetQNH(AIRSPACE_ALT &alt)
{
  if (alt.Base == abFL)
    alt.Altitude = AltitudeToQNHAltitude(alt.FL * 100.0 / TOFEET);
}

void
AirspaceDatabase::SetQNH(double _qnh)
{
  if (_qnh == qnh)
    return;

  unsigned i;

  for (i = 0; i < NumberOfAirspaceAreas; i++) {
    ::SetQNH(AirspaceArea[i].Top);
    ::SetQNH(AirspaceArea[i].Base);
  }

  for (i = 0; i < NumberOfAirspaceCircles; i++) {
    ::SetQNH(AirspaceCircle[i].Top);
    ::SetQNH(AirspaceCircle[i].Base);
  }

  qnh = _qnh;
}

/**
 * Calculate the distance to the border of the AirspaceCircle
 * @param location Location of the point to be calculated
 * @param i Array id of the AirspaceCircle
 * @return The distance between the given location and the border of the
 * specified AirspaceCircle
 */
double
AirspaceDatabase::CircleDistance(const GEOPOINT &location,
                                 const unsigned i) const
{
  return Distance(location, AirspaceCircle[i].Location) -
    AirspaceCircle[i].Radius;
}

/**
 * Checks whether a longitude is between a certain
 * minimum and maximum
 * @param longitude Longitude to be checked
 * @param lon_min Minimum longitude
 * @param lon_max Maximum longitude
 * @return True if longitude is between the limits, False otherwise
 */
static bool
CheckInsideLongitude(double longitude,
                     const double lon_min, const double lon_max)
{
  if (lon_min<=lon_max) {
    // normal case
    return ((longitude>lon_min) && (longitude<lon_max));
  } else {
    // area goes across 180 degree boundary, so lon_min is +ve, lon_max is -ve (flipped)
    return ((longitude>lon_min) || (longitude<lon_max));
  }
}

/**
 * Checks whether the given location is inside the bounds of
 * the given airspace
 * @param airspace AirspaceMetadata to be checked
 * @param location Location to be checked
 * @return True if location is inside the bounds, False otherwise
 */
static bool
InsideBounds(const AirspaceMetadata &airspace, const GEOPOINT &location)
{
  return location.Latitude > airspace.bounds.miny &&
    location.Latitude < airspace.bounds.maxy &&
    CheckInsideLongitude(location.Longitude,
                         airspace.bounds.minx, airspace.bounds.maxx);
}

/**
 * Checks whether the given location is inside the specified AirspaceCircle
 * @param location Location to be checked
 * @param i Array id of the AirspaceCircle
 * @return True if location is inside the AirspaceCircle, False otherwise
 */
bool
AirspaceDatabase::InsideCircle(const GEOPOINT &location,
                               const unsigned i) const
{
  // First check bounds and if necessary check distance to the border
  return InsideBounds(AirspaceCircle[i], location) &&
    CircleDistance(location, i) < 0.0;
}

/**
 * Calculates the nearest AirspaceCircle and writes the distance and bearing
 * into the given variables
 * @param location Location to be checked
 * @param altitude (?)
 * @param terrain_altitude (?)
 * @param settings Settings object
 * @param nearestdistance Distance to the nearest AirspaceCircle (Pointer)
 * @param nearestbearing Bearing to the nearest AirspaceCircle (Pointer)
 * @param height (?)
 * @return Array id of the nearest AirspaceCircle
 */
int
AirspaceDatabase::NearestCircle(const GEOPOINT &location,
                                double altitude, double terrain_altitude,
                                const SETTINGS_COMPUTER &settings,
                                double *nearestdistance,
                                double *nearestbearing,
                                double *height) const
{
  unsigned int i;
//  int NearestIndex = 0;
  double Dist;
  int ifound = -1;

  if (NumberOfAirspaceCircles == 0) {
    return -1;
  }

  for (i = 0; i < NumberOfAirspaceCircles; i++) {
    bool iswarn;
    bool isdisplay;

    iswarn = (settings.iAirspaceMode[AirspaceCircle[i].Type] >= 2);
    isdisplay = ((settings.iAirspaceMode[AirspaceCircle[i].Type] % 2) > 0);

    if (!isdisplay || !iswarn) {
      // don't want warnings for this one
      continue;
    }

    // QUESTION TB: Isn't there a function for that?! Not sure where I saw it...
    double basealt = ToMSL(AirspaceCircle[i].Base, terrain_altitude);
    double topalt = ToMSL(AirspaceCircle[i].Top, terrain_altitude);

    bool altok;
    if (height) {
      altok = ((*height > basealt) && (*height < topalt));
    } else {
      altok = CheckAirspaceAltitude(basealt, topalt, altitude, settings);
    }

    if (altok) {
      Dist = CircleDistance(location, i);

      if (Dist < *nearestdistance) {
        *nearestdistance = Dist;
        *nearestbearing = Bearing(location, AirspaceCircle[i].Location);
        if (Dist < 0) {
          // no need to continue search, the location is inside the circle
          return i;
        }
        ifound = i;
      }
    }
  }
  return ifound;
}

// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
//    a Point is defined by its coordinates {int x, y;}

/**
 * Tests if a point (P2) is Left|On|Right of an infinite line through P0 and P1.
 * @param P0 First point on the line
 * @param P1 Second point on the line
 * @param P2 Point to be checked
 * @return > 0 means P2 left of the line
 * = 0 means P2 on the line
 * < 0 means P2 right of the line
 * @see The January 2001 Algorithm "Area of 2D and 3D Triangles and Polygons"
 */
inline static double isLeft(AIRSPACE_POINT P0, AIRSPACE_POINT P1,
    AIRSPACE_POINT P2) {
  return ((P1.Longitude - P0.Longitude) * (P2.Latitude - P0.Latitude)
      - (P2.Longitude - P0.Longitude) * (P1.Latitude - P0.Latitude));
}

/**
 * Does a winding number test for a point in a polygon
 * @param P Point to be checked
 * @param V Array of points that build the polygon
 * @param n Number of points in V
 * @return The winding number (=0 only if P is outside V[])
 */
static int wn_PnPoly(AIRSPACE_POINT P, AIRSPACE_POINT* V, int n) {
  int wn = 0; // the winding number counter

  // loop through all edges of the polygon
  for (int i = 0; i < n; i++) { // edge from V[i] to V[i+1]
    if (V[i].Latitude <= P.Latitude) { // start y <= P.Latitude
      if (V[i + 1].Latitude > P.Latitude) // an upward crossing
        if (isLeft(V[i], V[i + 1], P) > 0) // P left of edge
          ++wn; // have a valid up intersect
    } else { // start y > P.Latitude (no test needed)
      if (V[i + 1].Latitude <= P.Latitude) // a downward crossing
        if (isLeft(V[i], V[i + 1], P) < 0) // P right of edge
          --wn; // have a valid down intersect
    }
  }
  return wn;
}

/**
 * Checks whether the given location is inside the specified AirspaceArea
 * @param location Location to be checked
 * @param i Array id of the AirspaceArea
 * @return True if location is inside the AirspaceArea, False otherwise
 */
bool
AirspaceDatabase::InsideArea(const GEOPOINT &location, const unsigned i) const
{
  AIRSPACE_POINT thispoint;
  thispoint.Longitude = location.Longitude;
  thispoint.Latitude = location.Latitude;

  // first check if point is within bounding box
  return InsideBounds(AirspaceArea[i], location) &&
    // it is within, so now do detailed polygon test
    wn_PnPoly(thispoint, &AirspacePoint[AirspaceArea[i].FirstPoint],
              AirspaceArea[i].NumPoints - 1) != 0;
}

/**
 * Calculates the CrossTrackError in screen coordinates
 *
 * Imagine an infinite line between loc1 and loc2.
 * You are standing at loc3. loc4 would now be the point
 * on the mentioned line closest to you.
 *
 * Original comment:
 *
 * this one uses screen coordinates to avoid as many
 * trig functions as possible.. it means it is approximate
 * but for our use it is ok.
 * @param loc1 First point of the line
 * @param loc2 Second point of the line
 * @param loc3 Location of the viewer
 * @param loc4 Closest point of the line (Pointer)
 * @param map_projection MapWindowProjection object
 * @return The Distance between the viewer and the closest point
 * of the line
 * @see ScreenClosestPoint
 */
static double
ScreenCrossTrackError(GEOPOINT loc1,
                      GEOPOINT loc2,
                      GEOPOINT loc3,
                      GEOPOINT *loc4,
                      const MapWindowProjection &map_projection)
{
  POINT p1, p2, p3, p4;

  map_projection.LonLat2Screen(loc1, p1);
  map_projection.LonLat2Screen(loc2, p2);
  map_projection.LonLat2Screen(loc3, p3);

  ScreenClosestPoint(p1, p2, p3, &p4, 0);

  map_projection.Screen2LonLat(p4.x, p4.y, *loc4);

  // compute accurate distance
  return Distance(loc3, *loc4);
}

double
AirspaceDatabase::RangeArea(const GEOPOINT &location, const int i,
                            double *bearing,
                            const MapWindowProjection &map_projection) const
{
  // find nearest distance to line segment
  int j;
  double dist= 0;
  double nearestdistance = dist;
  double nearestbearing = *bearing;
  GEOPOINT loc4;
  for (j=0; j<(int)AirspaceArea[i].NumPoints-1; j++) {

    int p1 = AirspaceArea[i].FirstPoint+j;
    int p2 = AirspaceArea[i].FirstPoint+j+1;

    dist = ScreenCrossTrackError(AirspacePoint[p1],
                                 AirspacePoint[p2],
                                 location,
                                 &loc4,
                                 map_projection);
    if ((dist<nearestdistance)||(j==0)) {
      nearestdistance = dist;
      nearestbearing = Bearing(location, loc4);
    }
  }
  *bearing = nearestbearing;
  return nearestdistance;
}

int
AirspaceDatabase::NearestArea(const GEOPOINT &location,
                              double altitude, double terrain_altitude,
                              const SETTINGS_COMPUTER &settings,
                              const MapWindowProjection& map_projection,
                              double *nearestdistance, double *nearestbearing,
                              double *height) const
{
  unsigned i;
  int ifound = -1;
  bool inside=false;
  // location of point the target is abeam along line in airspace area

  if (NumberOfAirspaceAreas == 0) {
    return -1;
  }

  for (i = 0; i < NumberOfAirspaceAreas; i++) {
    bool iswarn;
    bool isdisplay;

    iswarn = (settings.iAirspaceMode[AirspaceArea[i].Type] >= 2);
    isdisplay = ((settings.iAirspaceMode[AirspaceArea[i].Type] % 2) > 0);

    if (!isdisplay || !iswarn) {
      // don't want warnings for this one
      continue;
    }

    double basealt = ToMSL(AirspaceArea[i].Base, terrain_altitude);
    double topalt = ToMSL(AirspaceArea[i].Top, terrain_altitude);

    bool altok;
    if (!height) {
      altok = CheckAirspaceAltitude(basealt, topalt, altitude, settings);
    } else {
      altok = ((*height < topalt) && (*height > basealt));
    }
    if(altok) {
      inside = InsideArea(location, i);
      double dist, bearing;

      dist = RangeArea(location, i, &bearing, map_projection);

      if (dist < *nearestdistance) {
        *nearestdistance = dist;
        *nearestbearing = bearing;
        ifound = i;
      }
      if (inside) {
        // no need to continue the search
        *nearestdistance = -(*nearestdistance);
        return i;
      }
    }
  }
  // not inside any airspace, so return closest one
  return ifound;
}

/**
 * Checks whether two lines defined by four points intersect or not
 * @param x1 x-Coordinate of the first point of the first line
 * @param y1 y-Coordinate of the first point of the first line
 * @param dx x-Distance between first and second point of the first line
 * @param dy y-Distance between first and second point of the first line
 * @param x3 x-Coordinate of the first point of the second line
 * @param y3 y-Coordinate of the first point of the second line
 * @param x4 x-Coordinate of the second point of the second line
 * @param y4 y-Coordinate of the second point of the second line
 * @param u ?
 * @return 1 if lines intersect within bounds, 0 otherwise
 * @see http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
 */
int line_line_intersection (const double x1, const double y1,
			    const double dx, const double dy,
			    const double x3, const double y3,
			    const double x4, const double y4,
			    double *u) {
  /*
  double a = sqr(dx)+sqr(dy);
  if (a <= 0) {
    return 0;
  }
  */
  double denom = (y4-y3)*dx-(x4-x3)*dy;
  if (denom == 0) {
    // lines are parallel
    return 0;
  }
  double ua = ((x4-x3)*(y1-y3)-(y4-y3)*(x1-x3))/denom;
  if ((ua<0) || (ua>1.0)) {
    // outside first line
    return 0;
  } else {
    double ub = (dx*(y1-y3)-dy*(x1-x3))/denom;
    if ((ub<0) || (ub>1.0)) {
      // outside second line
      return 0;
    } else {
      // inside both lines
      u[0] = ua;
      return 1;
    }
  }
}

/**
 * Checks whether a line defined by two points
 * intersects with a rect defined by its bounds
 * @param x1 x-Coordinate of the first point of the line
 * @param y1 y-Coordinate of the first point of the line
 * @param dx x-Distance between first and second point of the line
 * @param dy y-Distance between first and second point of the line
 * @param bounds Pointer to the bounds of the rect
 * @return True if line intersects with rect, False otherwise
 */
bool line_rect_intersection(const double x1, const double y1, const double dx,
    const double dy, rectObj *bounds) {
  double u;

  // bottom line
  if (line_line_intersection(x1, y1, dx, dy, bounds->minx, bounds->miny,
      bounds->maxx, bounds->miny, &u))
    return true;

  // left line
  if (line_line_intersection(x1, y1, dx, dy, bounds->minx, bounds->miny,
      bounds->minx, bounds->maxy, &u))
    return true;

  // top line
  if (line_line_intersection(x1, y1, dx, dy, bounds->minx, bounds->maxy,
      bounds->maxx, bounds->maxy, &u))
    return true;

  // right line
  if (line_line_intersection(x1, y1, dx, dy, bounds->maxx, bounds->miny,
      bounds->maxx, bounds->maxy, &u))
    return true;

  return false;
}

void
AirspaceDatabase::ScanLine(const GEOPOINT *locs, const double *heights,
                           int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X]) const
{
  int i, j;
  unsigned int k;
  double Dist;
  double x1 = locs[0].Longitude;
  double dx = locs[AIRSPACE_SCANSIZE_X - 1].Longitude - x1;
  double y1 = locs[0].Latitude;
  double dy = locs[AIRSPACE_SCANSIZE_X - 1].Latitude - y1;
  double h_min = heights[0];
  double h_max = heights[AIRSPACE_SCANSIZE_H - 1];

  rectObj lineRect;

  lineRect.minx = min(x1, x1+dx);
  lineRect.maxx = max(x1, x1+dx);
  lineRect.miny = min(y1, y1+dy);
  lineRect.maxy = max(y1, y1+dy);

  for (k = 0; k < NumberOfAirspaceCircles; k++) {

    // ignore if outside scan height
    if (!((h_max <= AirspaceCircle[k].Base.Altitude) || (h_min
        >= AirspaceCircle[k].Top.Altitude))) {

      // ignore if scan line doesn't intersect bounds
      if (msRectOverlap(&lineRect, &AirspaceCircle[k].bounds)
          && line_rect_intersection(x1, y1, dx, dy, &AirspaceCircle[k].bounds)) {

        for (i = 0; i < AIRSPACE_SCANSIZE_X; i++) {
          if (InsideBounds(AirspaceCircle[k], locs[i])) {
            Dist = Distance(locs[i], AirspaceCircle[k].Location)
                - AirspaceCircle[k].Radius;

            if (Dist < 0) {
              for (j = 0; j < AIRSPACE_SCANSIZE_H; j++) {
                if ((heights[j] > AirspaceCircle[k].Base.Altitude)
                    && (heights[j] < AirspaceCircle[k].Top.Altitude)) {
                  airspacetype[j][i] = AirspaceCircle[k].Type;
                } // inside height
              } // finished scanning height
            } // inside
          } // in bound
        } // finished scanning range
      } // line intersects
    } // within height
  } // finished scanning circles

  for (k = 0; k < NumberOfAirspaceAreas; k++) {

    // ignore if outside scan height
    if (!((h_max <= AirspaceArea[k].Base.Altitude) || (h_min
        >= AirspaceArea[k].Top.Altitude))) {

      // ignore if scan line doesn't intersect bounds
      if (msRectOverlap(&lineRect, &AirspaceArea[k].bounds)
          && line_rect_intersection(x1, y1, dx, dy, &AirspaceArea[k].bounds)) {

        for (i = 0; i < AIRSPACE_SCANSIZE_X; i++) {

          if (InsideBounds(AirspaceArea[k], locs[i])) {
            AIRSPACE_POINT thispoint = locs[i];

            if (wn_PnPoly(thispoint,
                &AirspacePoint[AirspaceArea[k].FirstPoint],
                AirspaceArea[k].NumPoints - 1) != 0) {
              for (j = 0; j < AIRSPACE_SCANSIZE_H; j++) {
                if ((heights[j] > AirspaceArea[k].Base.Altitude) && (heights[j]
                    < AirspaceArea[k].Top.Altitude)) {
                  airspacetype[j][i] = AirspaceArea[k].Type;
                } // inside height
              } // finished scanning height
            } // inside
          } // in bound
        } // finished scanning range
      } // line intersects
    } // within height
  } // finished scanning areas
}

#ifndef NDEBUG

static void
Dump(FILE *fp, const TCHAR *label, const AIRSPACE_ALT &altitude)
{
    switch (altitude.Base){
      case abUndef:
        _ftprintf(fp, TEXT("  %s : %.0f[m] %.0f[ft] [?]\r\n"),
                  label, altitude.Altitude, altitude.Altitude * TOFEET);
      break;
      case abMSL:
        _ftprintf(fp, TEXT("  %s : %.0f[m] %.0f[ft] [MSL]\r\n"),
                  label, altitude.Altitude, altitude.Altitude * TOFEET);
      break;
      case abAGL:
        _ftprintf(fp, TEXT("  %s : %.0f[m] %.0f[ft] [AGL]\r\n"),
                  label, altitude.AGL, altitude.AGL * TOFEET);
      break;
      case abFL:
        _ftprintf(fp, TEXT("  %s : FL %.0f (%.0f[m] %.0f[ft])\r\n"),
                  label, altitude.FL, altitude.Altitude,
                  altitude.Altitude * TOFEET);
      break;
    }
}

void
AirspaceDatabase::Dump(FILE *fp) const
{
  for (unsigned i = 0; i < NumberOfAirspaceAreas; ++i) {

    _ftprintf(fp, TEXT("*** Aera id: %d %s "), i, AirspaceArea[i].Name);

    switch (AirspaceArea[i].Type){
      case RESTRICT:
        _ftprintf(fp, TEXT("Restricted")); break;
      case PROHIBITED:
        _ftprintf(fp, TEXT("Prohibited")); break;
      case DANGER:
        _ftprintf(fp, TEXT("Danger Area")); break;
      case CLASSA:
        _ftprintf(fp, TEXT("Class A")); break;
      case CLASSB:
        _ftprintf(fp, TEXT("Class B")); break;
      case CLASSC:
        _ftprintf(fp, TEXT("Class C")); break;
      case CLASSD:
        _ftprintf(fp, TEXT("Class D")); break;
      case CLASSE:
        _ftprintf(fp, TEXT("Class E")); break;
      case CLASSF:
        _ftprintf(fp, TEXT("Class F")); break;
      case NOGLIDER:
        _ftprintf(fp, TEXT("No Glider")); break;
      case CTR:
        _ftprintf(fp, TEXT("CTR")); break;
      case WAVE:
        _ftprintf(fp, TEXT("Wave")); break;
      default:
        _ftprintf(fp, TEXT("Unknown"));
    }

    _ftprintf(fp, TEXT(")\r\n"));

    ::Dump(fp, _T("Top "), AirspaceArea[i].Top);
    ::Dump(fp, _T("Base"), AirspaceArea[i].Base);

    _ftprintf(fp, TEXT("\r\n"));
  }

  for (unsigned i = 0; i < NumberOfAirspaceCircles; ++i) {

    _ftprintf(fp, TEXT("\r\n*** Circle id: %d %s ("), i, AirspaceCircle[i].Name);

    switch (AirspaceArea[i].Type){
      case RESTRICT:
        _ftprintf(fp, TEXT("Restricted")); break;
      case PROHIBITED:
        _ftprintf(fp, TEXT("Prohibited")); break;
      case DANGER:
        _ftprintf(fp, TEXT("Danger Area")); break;
      case CLASSA:
        _ftprintf(fp, TEXT("Class A")); break;
      case CLASSB:
        _ftprintf(fp, TEXT("Class B")); break;
      case CLASSC:
        _ftprintf(fp, TEXT("Class C")); break;
      case CLASSD:
        _ftprintf(fp, TEXT("Class D")); break;
      case CLASSE:
        _ftprintf(fp, TEXT("Class E")); break;
      case CLASSF:
        _ftprintf(fp, TEXT("Class F")); break;
      case NOGLIDER:
        _ftprintf(fp, TEXT("No Glider")); break;
      case CTR:
        _ftprintf(fp, TEXT("CTR")); break;
      case WAVE:
        _ftprintf(fp, TEXT("Wave")); break;
      default:
        _ftprintf(fp, TEXT("Unknown"));
    }

    _ftprintf(fp, TEXT(")\r\n"));

    ::Dump(fp, _T("Top "), AirspaceCircle[i].Top);
    ::Dump(fp, _T("Base"), AirspaceCircle[i].Base);

    _ftprintf(fp, TEXT("\r\n"));

  }
}
#endif
