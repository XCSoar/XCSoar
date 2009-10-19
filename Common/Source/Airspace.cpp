/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Airspace.h"
#include "MapWindowProjection.hpp"
#include "Math/Earth.hpp"
#include "Math/Screen.hpp"
#include "Math/Units.h"
#include "Math/Pressure.h"
#include "SettingsAirspace.hpp"
#include "Interface.hpp" // JMW remove (need to separate out blackboard)

//#include <windows.h>
//#include <commctrl.h>
//#include <math.h>

#include "wcecompat/ts_string.h"
#include "Compatibility/string.h"

#include <assert.h>


AIRSPACE_AREA *AirspaceArea;
AIRSPACE_POINT *AirspacePoint;
POINT *AirspaceScreenPoint;
AIRSPACE_CIRCLE *AirspaceCircle;
unsigned int NumberOfAirspacePoints;
unsigned int NumberOfAirspaceAreas;
unsigned int NumberOfAirspaceCircles;

/**
 * Deletes all airspaces in memory
 */
void DeleteAirspace() {
  NumberOfAirspacePoints = 0;
  NumberOfAirspaceAreas = 0;
  NumberOfAirspaceCircles = 0;

  if(AirspaceArea != NULL)   {
    LocalFree((HLOCAL)AirspaceArea);
    AirspaceArea = NULL;
  }
  if(AirspacePoint != NULL)  {
    LocalFree((HLOCAL)AirspacePoint);
    AirspacePoint = NULL;
  }
  if(AirspaceScreenPoint != NULL)  {
    LocalFree((HLOCAL)AirspaceScreenPoint);
    AirspaceScreenPoint = NULL;
  }
  if(AirspaceCircle != NULL) {
    AirspaceCircle = NULL;
    LocalFree((HLOCAL)AirspaceCircle);
  }
}

/**
 * Returns distance between location and AirspaceCircle border
 * @param location Location used for calculation
 * @param i Array id of the AirspaceCircle
 * @return Distance between location and airspace border
 */
double RangeAirspaceCircle(const GEOPOINT &location,
			   const int i) {
  return Distance(location, AirspaceCircle[i].Location) - AirspaceCircle[i].Radius;
}

/**
 * Checks whether a longitude is between a certain
 * minimum and maximum
 * @param longitude Longitude to be checked
 * @param lon_min Minimum longitude
 * @param lon_max Maximum longitude
 * @return True if longitude is between the limits, False otherwise
 */
bool CheckInsideLongitude(double longitude,
                          const double lon_min, const double lon_max) {
  if (lon_min<=lon_max) {
    // normal case
    return ((longitude>lon_min) && (longitude<lon_max));
  } else {
    // area goes across 180 degree boundary, so lon_min is +ve, lon_max is -ve (flipped)
    return ((longitude>lon_min) || (longitude<lon_max));
  }
}

/**
 * Checks whether the given location is inside
 * of a certain AirspaceCircle defined by i
 * @param location Location to be checked
 * @param i Array id of the AirspaceCircle
 * @return True if location is in AirspaceCircle, False otherwise
 */
bool InsideAirspaceCircle(const GEOPOINT &location,
                          const int i) {

  if ((location.Latitude> AirspaceCircle[i].bounds.miny) &&
      (location.Latitude< AirspaceCircle[i].bounds.maxy) &&
      CheckInsideLongitude(location.Longitude,
                           AirspaceCircle[i].bounds.minx,
                           AirspaceCircle[i].bounds.maxx)) {

    if (RangeAirspaceCircle(location, i)<0) {
      return true;
    }
  }
  return false;
}

/* unused
int FindAirspaceCircle(const GEOPOINT &location, bool visibleonly)
{
  unsigned i;
  double basealt;
  double topalt;
 // int NearestIndex = 0;

  if(NumberOfAirspaceCircles == 0)
    {
      return -1;
    }

  for(i=0;i<NumberOfAirspaceCircles;i++) {
    if (iAirspaceMode[AirspaceCircle[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    if(AirspaceCircle[i].Visible || (!visibleonly)) {

      if (AirspaceCircle[i].Base.Base != abAGL) {
          basealt = AirspaceCircle[i].Base.Altitude;
        } else {
          basealt = AirspaceCircle[i].Base.AGL + XCSoarInterface::Calculated().TerrainAlt;
        }
      if (AirspaceCircle[i].Top.Base != abAGL) {
          topalt = AirspaceCircle[i].Top.Altitude;
        } else {
          topalt = AirspaceCircle[i].Top.AGL + XCSoarInterface::Calculated().TerrainAlt;
        }
      if(CheckAirspaceAltitude(basealt, topalt, settings)) {
        if (InsideAirspaceCircle(location,i)) {
	  return i;
        }
      }
    }
  }
  return -1;
}
*/

/**
 * Checks whether to display an airspace depending
 * on the AltitudeMode setting and the altitude
 * @param Base Airspace base altitude
 * @param Top Airspace top altitude
 * @param settings Pointer to the settings
 * @return True if airspace is supposed to be drawn, False otherwise
 */
bool CheckAirspaceAltitude(const double &Base, const double &Top,
    const SETTINGS_COMPUTER &settings) {
  double alt;

  // TODO remove dependency!
  if (XCSoarInterface::Basic().BaroAltitudeAvailable) {
    alt = XCSoarInterface::Basic().BaroAltitude;
  } else {
    alt = XCSoarInterface::Basic().Altitude;
  }

  switch (settings.AltitudeMode) {
  case ALLON:
    return true;

  case CLIP:
    if (Base < settings.ClipAltitude)
      return true;
    else
      return false;

  case AUTO:
    if ((alt > (Base - settings.AltWarningMargin)) && (alt < (Top
        + settings.AltWarningMargin)))
      return true;
    else
      return false;

  case ALLBELOW:
    if ((Base - settings.AltWarningMargin) < alt)
      return true;
    else
      return false;

  case INSIDE:
    if ((alt >= (Base)) && (alt < (Top)))
      return true;
    else
      return false;

  case ALLOFF:
    return false;
  }

  return true;
}

/**
 * Corrects the FL-based airspaces with the new QNH
 * @param newQNH
 */
// TODO: hack, should be replaced with a data change notifier in the future...
void AirspaceQnhChangeNotify(double newQNH) {
  int i;
  AIRSPACE_ALT *Alt;

  if (newQNH != airspace_QNH) {
    for (i = 0; i < (int) NumberOfAirspaceAreas; i++) {
      Alt = &AirspaceArea[i].Top;

      if (Alt->Base == abFL) {
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100) / TOFEET);
      }

      Alt = &AirspaceArea[i].Base;

      if (Alt->Base == abFL) {
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100) / TOFEET);
      }
    }

    for (i = 0; i < (int) NumberOfAirspaceCircles; i++) {
      Alt = &AirspaceCircle[i].Top;

      if (Alt->Base == abFL) {
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100) / TOFEET);
      }

      Alt = &AirspaceCircle[i].Base;

      if (Alt->Base == abFL) {
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100) / TOFEET);
      }
    }

    airspace_QNH = newQNH;
  }
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
 * Checks whether a given location is inside the
 * AirspaceArea defined by i
 * @param location Location to be checked
 * @param i Array id of the AirspaceArea
 * @return True if location is inside the AirspaceArea, False otherwise
 */
bool InsideAirspaceArea(const GEOPOINT &location,
                        const int i) {
  AIRSPACE_POINT thispoint;
  thispoint.Longitude = location.Longitude;
  thispoint.Latitude = location.Latitude;

  // first check if point is within bounding box
  if (
      (location.Latitude> AirspaceArea[i].bounds.miny)&&
      (location.Latitude< AirspaceArea[i].bounds.maxy)&&
      CheckInsideLongitude(location.Longitude,
                           AirspaceArea[i].bounds.minx,
                           AirspaceArea[i].bounds.maxx)) {

    CheckAirspacePoint(AirspaceArea[i].FirstPoint);

    // it is within, so now do detailed polygon test
    if (wn_PnPoly(thispoint,
		  &AirspacePoint[AirspaceArea[i].FirstPoint],
		  AirspaceArea[i].NumPoints-1) != 0) {
      // we are inside the i'th airspace area
      return true;
    }
  }
  return false;
}

/*
int FindAirspaceArea(double Longitude,double Latitude, bool visibleonly)
{
  unsigned i;
  double basealt;
  double topalt;

  if(NumberOfAirspaceAreas == 0)
    {
      return -1;
    }
  for(i=0;i<NumberOfAirspaceAreas;i++) {
    if (iAirspaceMode[AirspaceArea[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    if(AirspaceArea[i].Visible || (!visibleonly)) {

      if (AirspaceArea[i].Base.Base != abAGL) {
          basealt = AirspaceArea[i].Base.Altitude;
      } else {
          basealt = AirspaceArea[i].Base.AGL + XCSoarInterface::Calculated().TerrainAlt;
      }
      if (AirspaceArea[i].Top.Base != abAGL) {
          topalt = AirspaceArea[i].Top.Altitude;
      } else {
          topalt = AirspaceArea[i].Top.AGL + XCSoarInterface::Calculated().TerrainAlt;
      }
      if(CheckAirspaceAltitude(basealt, topalt)) {
        if (InsideAirspaceArea(Longitude,Latitude,i)) {
          return i;
        }
      }
    }
  }
  // not inside any airspace
  return -1;
}
*/

/**
 * Finds the nearest AirspaceCircle
 * @param location Location where to check
 * @param settings Pointer to the settings object
 * @param nearestdistance Pointer in which the distance
 * to the nearest circle will be written in
 * @param nearestbearing Pointer in which the bearing
 * to the nearest circle will be written in
 * @param height If != NULL only airspaces are considered
 * where the height is between base and top of the airspace
 * @return Array id of the nearest AirspaceCircle
 */
static int
FindNearestAirspaceCircle(const GEOPOINT &location,
                          const SETTINGS_COMPUTER &settings,
                          double *nearestdistance,
                          double *nearestbearing,
                          double *height=NULL)
{
  unsigned int i;
//  int NearestIndex = 0;
  double Dist;
  int ifound = -1;

  if(NumberOfAirspaceCircles == 0) {
      return -1;
  }

  for(i=0;i<NumberOfAirspaceCircles;i++) {
    bool iswarn;
    bool isdisplay;
    double basealt;
    double topalt;

    iswarn = (settings.iAirspaceMode[AirspaceCircle[i].Type]>=2);
    isdisplay = ((settings.iAirspaceMode[AirspaceCircle[i].Type]%2)>0);

    if (!isdisplay || !iswarn) {
      // don't want warnings for this one
      continue;
    }

    // QUESTION TB: Isn't there a function for that?! Not sure where I saw it...
    if (AirspaceCircle[i].Base.Base != abAGL) {
      basealt = AirspaceCircle[i].Base.Altitude;
    } else {
      basealt = AirspaceCircle[i].Base.AGL + XCSoarInterface::Calculated().TerrainAlt;
    }
    if (AirspaceCircle[i].Top.Base != abAGL) {
      topalt = AirspaceCircle[i].Top.Altitude;
    } else {
      topalt = AirspaceCircle[i].Top.AGL + XCSoarInterface::Calculated().TerrainAlt;
    }

    bool altok;
    if (height) {
      altok = ((*height > basealt) && (*height < topalt));
    } else {
      altok = CheckAirspaceAltitude(basealt, topalt, settings)==true;
    }
    if(altok) {

      Dist = RangeAirspaceCircle(location, i);

      if (Dist < *nearestdistance) {
        *nearestdistance = Dist;
        *nearestbearing = Bearing(location, AirspaceCircle[i].Location);
        if (Dist < 0) {
          // no need to continue search, inside
          return i;
        }
        ifound = i;
      }
    }
  }
  return ifound;
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

/**
 * Calculates the distance to the border of the
 * AirspaceArea defined by i
 * @param location Location used for calculation
 * @param i Array id of the AirspaceArea
 * @param bearing Bearing to the closest point of the
 * airspace (Pointer)
 * @param map_projection MapWindowProjection object
 * @return Distance to the closest point of the airspace
 */
double RangeAirspaceArea(const GEOPOINT &location,
			 const int i, double *bearing,
			 const MapWindowProjection &map_projection)
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
    CheckAirspacePoint(p1);
    CheckAirspacePoint(p2);

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

/**
 * Finds the nearest AirspaceArea
 * @param location Location where to check
 * @param settings Pointer to the settings object
 * @param map_projection MapWindowProjection object
 * @param nearestdistance Pointer in which the distance
 * to the nearest area will be written in
 * @param nearestbearing Pointer in which the bearing
 * to the nearest area will be written in
 * @param height If != NULL only airspaces are considered
 * where the height is between base and top of the airspace
 * @return Array id of the nearest AirspaceArea
 */
static int
FindNearestAirspaceArea(const GEOPOINT &location,
                        const SETTINGS_COMPUTER &settings,
                        const MapWindowProjection& map_projection,
                        double *nearestdistance, double *nearestbearing,
                        double *height=NULL)
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
    double basealt;
    double topalt;

    iswarn = (settings.iAirspaceMode[AirspaceArea[i].Type] >= 2);
    isdisplay = ((settings.iAirspaceMode[AirspaceArea[i].Type] % 2) > 0);

    if (!isdisplay || !iswarn) {
      // don't want warnings for this one
      continue;
    }

    if (AirspaceArea[i].Base.Base != abAGL) {
      basealt = AirspaceArea[i].Base.Altitude;
    } else {
      basealt = AirspaceArea[i].Base.AGL + XCSoarInterface::Calculated().TerrainAlt;
    }
    if (AirspaceArea[i].Top.Base != abAGL) {
      topalt = AirspaceArea[i].Top.Altitude;
    } else {
      topalt = AirspaceArea[i].Top.AGL + XCSoarInterface::Calculated().TerrainAlt;
    }

    bool altok;
    if (!height) {
      altok = CheckAirspaceAltitude(basealt, topalt, settings)==true;
    } else {
      altok = ((*height < topalt) && (*height > basealt));
    }
    if(altok) {
      inside = InsideAirspaceArea(location, i);
      double dist, bearing;

      dist = RangeAirspaceArea(location, i, &bearing, map_projection);

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




// Finds nearest airspace (whether circle or area) to the specified point. Returns -1 in foundcircle or foundarea if circle or area is not found. Otherwise, returns index of the circle or area that is closest to the specified point. Also returns the distance and bearing to the boundary of the airspace. Distance < 0 means interior. This only searches within a range of 100km of the target
/**
 * Finds nearest airspace (whether circle or area)
 * to the specified point.
 *
 * Returns -1 in foundcircle or foundarea if circle or
 * area is not found. Otherwise, returns index of the
 * circle or area that is closest to the specified point.
 *
 * Also returns the distance and bearing to the boundary
 * of the airspace.
 * (Distance < 0 means interior)
 *
 * This only searches within a range of 100km of the target.
 * @param location Location where to search
 * @param settings Settings object
 * @param map_projection MapWindowProjection object
 * @param nearestdistance Distance to nearest airspace (Pointer)
 * @param nearestbearing Bearing to nearest airspace (Pointer)
 * @param foundcircle Array id of nearest AirspaceCircle
 * @param foundarea Array if of nearest AirspaceArea
 * @param height If != NULL only airspaces are considered
 * where the height is between base and top of the airspace
 */
void FindNearestAirspace(const GEOPOINT &location,
    const SETTINGS_COMPUTER &settings,
    const MapWindowProjection& map_projection, double *nearestdistance,
    double *nearestbearing, int *foundcircle, int *foundarea, double *height) {

  // TODO enhancement: return also the vertical separation
  double nearestd1 = 100000; // 100km
  double nearestd2 = 100000; // 100km
  double nearestb1 = 0;
  double nearestb2 = 0;

  *foundcircle = FindNearestAirspaceCircle(location, settings, &nearestd1,
      &nearestb1, height);

  *foundarea = FindNearestAirspaceArea(location, settings, map_projection,
      &nearestd2, &nearestb2, height);

  if ((*foundcircle >= 0) && (*foundarea < 0)) {
    *nearestdistance = nearestd1;
    *nearestbearing = nearestb1;
    *foundarea = -1;
    return;
  }
  if ((*foundarea >= 0) && (*foundcircle < 0)) {
    *nearestdistance = nearestd2;
    *nearestbearing = nearestb2;
    *foundcircle = -1;
    return;
  }

  if (nearestd1 < nearestd2) {
    if (nearestd1 < 100000) {
      *nearestdistance = nearestd1;
      *nearestbearing = nearestb1;
      *foundarea = -1;
    }
  } else {
    if (nearestd2 < 100000) {
      *nearestdistance = nearestd2;
      *nearestbearing = nearestb2;
      *foundcircle = -1;
    }
  }

  return;
}

/**
 * Checks whether two lines defined by four points
 * intersect or not
 * @param x1 x-Coordinate of the first point of the
 * first line
 * @param y1 y-Coordinate of the first point of the
 * first line
 * @param dx x-Distance between first and second point
 * of the first line
 * @param dy y-Distance between first and second point
 * of the first line
 * @param x3 x-Coordinate of the first point of the
 * second line
 * @param y3 y-Coordinate of the first point of the
 * second line
 * @param x4 x-Coordinate of the second point of the
 * second line
 * @param y4 y-Coordinate of the second point of the
 * second line
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
 * @param x1 x-Coordinate of the first point of the
 * line
 * @param y1 y-Coordinate of the first point of the
 * line
 * @param dx x-Distance between first and second point
 * of the line
 * @param dy y-Distance between first and second point
 * of the line
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


void ScanAirspaceLine(const GEOPOINT *locs, const double *heights,
    int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X]) {

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
          if ((locs[i].Latitude > AirspaceCircle[k].bounds.miny)
              && (locs[i].Latitude < AirspaceCircle[k].bounds.maxy)
              && CheckInsideLongitude(locs[i].Longitude,
                  AirspaceCircle[k].bounds.minx, AirspaceCircle[k].bounds.maxx)) {

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

          if ((locs[i].Latitude > AirspaceArea[k].bounds.miny)
              && (locs[i].Latitude < AirspaceArea[k].bounds.maxy)
              && CheckInsideLongitude(locs[i].Longitude,
                  AirspaceArea[k].bounds.minx, AirspaceArea[k].bounds.maxx)) {
            AIRSPACE_POINT thispoint = locs[i];

            CheckAirspacePoint(AirspaceArea[k].FirstPoint);

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

