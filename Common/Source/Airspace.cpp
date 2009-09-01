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
#include "Blackboard.hpp"
#include "MapWindowProjection.hpp"
#include "Math/Earth.hpp"
#include "Math/Screen.hpp"
#include "Math/Units.h"
#include "Math/Pressure.h"
#include "SettingsAirspace.hpp"

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

int AIRSPACEWARNINGS = TRUE;
int WarningTime = 30;
int AcknowledgementTime = 30;
int AltitudeMode = ALLON;
int ClipAltitude = 1000;
int AltWarningMargin = 100;

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


double RangeAirspaceCircle(const double &longitude,
			   const double &latitude,
			   int i) {
  double distance;
  DistanceBearing(latitude,longitude,
                  AirspaceCircle[i].Latitude,
                  AirspaceCircle[i].Longitude,
                  &distance, NULL);
  return distance-AirspaceCircle[i].Radius;
}


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


bool InsideAirspaceCircle(const double &longitude,
			    const double &latitude,
			    int i) {

  if ((latitude> AirspaceCircle[i].bounds.miny) &&
      (latitude< AirspaceCircle[i].bounds.maxy) &&
      CheckInsideLongitude(longitude, AirspaceCircle[i].bounds.minx,
                           AirspaceCircle[i].bounds.maxx)) {

    if (RangeAirspaceCircle(longitude, latitude, i)<0) {
      return true;
    }
  }
  return false;
}


int FindAirspaceCircle(double Longitude,double Latitude, bool visibleonly)
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
          basealt = AirspaceCircle[i].Base.AGL + CALCULATED_INFO.TerrainAlt;
        }
      if (AirspaceCircle[i].Top.Base != abAGL) {
          topalt = AirspaceCircle[i].Top.Altitude;
        } else {
          topalt = AirspaceCircle[i].Top.AGL + CALCULATED_INFO.TerrainAlt;
        }
      if(CheckAirspaceAltitude(basealt, topalt)) {
        if (InsideAirspaceCircle(Longitude,Latitude,i)) {
	  return i;
        }
      }
    }
  }
  return -1;
}


BOOL CheckAirspaceAltitude(const double &Base, const double &Top)
{
  double alt;
  if (GPS_INFO.BaroAltitudeAvailable) {
    alt = GPS_INFO.BaroAltitude;
  } else {
    alt = GPS_INFO.Altitude;
  }

  switch (AltitudeMode)
    {
    case ALLON : return TRUE;

    case CLIP :
      if(Base < ClipAltitude)
	return TRUE;
      else
	return FALSE;

    case AUTO:
      if( ( alt > (Base - AltWarningMargin) )
	  && ( alt < (Top + AltWarningMargin) ))
	return TRUE;
      else
	return FALSE;

    case ALLBELOW:
      if(  (Base - AltWarningMargin) < alt )
	return  TRUE;
      else
	return FALSE;
    case INSIDE:
      if( ( alt >= (Base) ) && ( alt < (Top) ))
	return TRUE;
      else
        return FALSE;
    case ALLOFF : return FALSE;
    }
  return TRUE;
}

double airspace_QNH;

// hack, should be replaced with a data change notifier in the future...
void AirspaceQnhChangeNotify(double newQNH){

  int i;
  AIRSPACE_ALT *Alt;

  if (newQNH != airspace_QNH){

    for(i=0;i<(int)NumberOfAirspaceAreas;i++) {

      Alt = &AirspaceArea[i].Top;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }

      Alt = &AirspaceArea[i].Base;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }
    }

    for(i=0;i<(int)NumberOfAirspaceCircles;i++) {

      Alt = &AirspaceCircle[i].Top;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }

      Alt = &AirspaceCircle[i].Base;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }
    }

    airspace_QNH = newQNH;

  }


}

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
isLeft( AIRSPACE_POINT P0, AIRSPACE_POINT P1, AIRSPACE_POINT P2 )
{
    return ( (P1.Longitude - P0.Longitude) * (P2.Latitude - P0.Latitude)
            - (P2.Longitude - P0.Longitude) * (P1.Latitude - P0.Latitude) );
}
//===================================================================

// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only if P is outside V[])
static int
wn_PnPoly( AIRSPACE_POINT P, AIRSPACE_POINT* V, int n )
{
    int    wn = 0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<n; i++) {   // edge from V[i] to V[i+1]
        if (V[i].Latitude <= P.Latitude) {         // start y <= P.Latitude
            if (V[i+1].Latitude > P.Latitude)      // an upward crossing
                if (isLeft( V[i], V[i+1], P) > 0)  // P left of edge
                    ++wn;            // have a valid up intersect
        }
        else {                       // start y > P.Latitude (no test needed)
            if (V[i+1].Latitude <= P.Latitude)     // a downward crossing
                if (isLeft( V[i], V[i+1], P) < 0)  // P right of edge
                    --wn;            // have a valid down intersect
        }
    }
    return wn;
}
//===================================================================


bool InsideAirspaceArea(const double &longitude,
			  const double &latitude,
			  int i) {
  AIRSPACE_POINT thispoint;
  thispoint.Longitude = longitude;
  thispoint.Latitude = latitude;

  // first check if point is within bounding box
  if (
      (latitude> AirspaceArea[i].bounds.miny)&&
      (latitude< AirspaceArea[i].bounds.maxy)&&
      CheckInsideLongitude(longitude,
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
          basealt = AirspaceArea[i].Base.AGL + CALCULATED_INFO.TerrainAlt;
      }
      if (AirspaceArea[i].Top.Base != abAGL) {
          topalt = AirspaceArea[i].Top.Altitude;
      } else {
          topalt = AirspaceArea[i].Top.AGL + CALCULATED_INFO.TerrainAlt;
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



/////////////////////////////////////////////////////////////////////////////////


int FindNearestAirspaceCircle(double longitude, double latitude,
			      double *nearestdistance,
			      double *nearestbearing,
			      double *nearestt,
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

    iswarn = (iAirspaceMode[AirspaceCircle[i].Type]>=2);
    isdisplay = ((iAirspaceMode[AirspaceCircle[i].Type]%2)>0);

    if (!isdisplay || !iswarn) {
      // don't want warnings for this one
      continue;
    }

    if (AirspaceCircle[i].Base.Base != abAGL) {
      basealt = AirspaceCircle[i].Base.Altitude;
    } else {
      basealt = AirspaceCircle[i].Base.AGL + CALCULATED_INFO.TerrainAlt;
    }
    if (AirspaceCircle[i].Top.Base != abAGL) {
      topalt = AirspaceCircle[i].Top.Altitude;
    } else {
      topalt = AirspaceCircle[i].Top.AGL + CALCULATED_INFO.TerrainAlt;
    }

    bool altok;
    if (height) {
      altok = ((*height > basealt) && (*height < topalt));
    } else {
      altok = CheckAirspaceAltitude(basealt, topalt)==TRUE;
    }
    if(altok) {

      Dist = RangeAirspaceCircle(longitude, latitude, i);

      if(Dist < *nearestdistance ) {
	  *nearestdistance = Dist;
          DistanceBearing(latitude,
                          longitude,
                          AirspaceCircle[i].Latitude,
                          AirspaceCircle[i].Longitude,
                          NULL, nearestbearing);
	  if (Dist<0) {
	    // no need to continue search, inside
	    return i;
	  }
	  ifound = i;
      }
    }
  }
  return ifound;
}


// this one uses screen coordinates to avoid as many trig functions
// as possible.. it means it is approximate but for our use it is ok.
double ScreenCrossTrackError(double lon1, double lat1,
			     double lon2, double lat2,
			     double lon3, double lat3,
			     double *lon4, double *lat4,
			     MapWindowProjection &map_projection) {
  POINT p1, p2, p3, p4;

  map_projection.LonLat2Screen(lon1, lat1, p1);
  map_projection.LonLat2Screen(lon2, lat2, p2);
  map_projection.LonLat2Screen(lon3, lat3, p3);

  ScreenClosestPoint(p1, p2, p3, &p4, 0);

  map_projection.Screen2LonLat(p4.x, p4.y, *lon4, *lat4);

  // compute accurate distance
  double tmpd;
  DistanceBearing(lat3, lon3, *lat4, *lon4, &tmpd, NULL);
  return tmpd;
}


double RangeAirspaceArea(const double &longitude,
			 const double &latitude,
			 int i, double *bearing,
			 MapWindowProjection &map_projection) {

  // find nearest distance to line segment
  int j;
  double dist= 0;
  double nearestdistance = dist;
  double nearestbearing = *bearing;
  double lon4, lat4;
  for (j=0; j<(int)AirspaceArea[i].NumPoints-1; j++) {

    int p1 = AirspaceArea[i].FirstPoint+j;
    int p2 = AirspaceArea[i].FirstPoint+j+1;
    CheckAirspacePoint(p1);
    CheckAirspacePoint(p2);

    dist = ScreenCrossTrackError(
				 AirspacePoint[p1].Longitude,
				 AirspacePoint[p1].Latitude,
				 AirspacePoint[p2].Longitude,
				 AirspacePoint[p2].Latitude,
				 longitude, latitude,
				 &lon4, &lat4,
				 map_projection);
    if ((dist<nearestdistance)||(j==0)) {
      nearestdistance = dist;

      DistanceBearing(latitude, longitude,
                      lat4, lon4, NULL,
                      &nearestbearing);
    }
  }
  *bearing = nearestbearing;
  return nearestdistance;
}





int FindNearestAirspaceArea(MapWindowProjection& map_projection,
			    double longitude,
			    double latitude,
			    double *nearestdistance,
			    double *nearestbearing,
			    double *nearestt,
			    double *height=NULL)
{
  unsigned i;
  int ifound = -1;
  bool inside=false;
  // location of point the target is abeam along line in airspace area

  if(NumberOfAirspaceAreas == 0)
    {
      return -1;
    }

  for(i=0;i<NumberOfAirspaceAreas;i++) {
    bool iswarn;
    bool isdisplay;
    double basealt;
    double topalt;

    iswarn = (iAirspaceMode[AirspaceArea[i].Type]>=2);
    isdisplay = ((iAirspaceMode[AirspaceArea[i].Type]%2)>0);

    if (!isdisplay || !iswarn) {
      // don't want warnings for this one
      continue;
    }

    if (AirspaceArea[i].Base.Base != abAGL) {
      basealt = AirspaceArea[i].Base.Altitude;
    } else {
      basealt = AirspaceArea[i].Base.AGL + CALCULATED_INFO.TerrainAlt;
    }
    if (AirspaceArea[i].Top.Base != abAGL) {
      topalt = AirspaceArea[i].Top.Altitude;
    } else {
      topalt = AirspaceArea[i].Top.AGL + CALCULATED_INFO.TerrainAlt;
    }

    bool altok;
    if (!height) {
      altok = CheckAirspaceAltitude(basealt, topalt)==TRUE;
    } else {
      altok = ((*height < topalt) && (*height > basealt));
    }
    if(altok) {
      inside = InsideAirspaceArea(longitude, latitude, i);
      double dist, bearing;

      dist = RangeAirspaceArea(longitude, latitude, i, &bearing, map_projection);

      if (dist< *nearestdistance) {
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




////////////////////////
//
// Finds nearest airspace (whether circle or area) to the specified point.
// Returns -1 in foundcircle or foundarea if circle or area is not found
// Otherwise, returns index of the circle or area that is closest to the specified point.
//
// Also returns the distance and bearing to the boundary of the airspace,
// (TODO enhancement: return also the vertical separation).
//
// Distance <0 means interior.
//
// This only searches within a range of 100km of the target

void FindNearestAirspace(MapWindowProjection& map_projection,
			 double longitude, double latitude,
			 double *nearestdistance, double *nearestbearing,
			 int *foundcircle, int *foundarea,
			 double *height)
{
  double nearestd1 = 100000; // 100km
  double nearestd2 = 100000; // 100km
  double nearestb1 = 0;
  double nearestb2 = 0;

  double nearestt = 100000;

  *foundcircle = FindNearestAirspaceCircle(longitude, latitude,
					   &nearestd1, &nearestb1,
					   &nearestt,
					   height);

  *foundarea = FindNearestAirspaceArea(map_projection,
				       longitude, latitude,
				       &nearestd2, &nearestb2,
				       &nearestt,
				       height);

  if ((*foundcircle>=0)&&(*foundarea<0)) {
      *nearestdistance = nearestd1;
      *nearestbearing = nearestb1;
      *foundarea = -1;
      return;
  }
  if ((*foundarea>=0)&&(*foundcircle<0)) {
      *nearestdistance = nearestd2;
      *nearestbearing = nearestb2;
      *foundcircle = -1;
      return;
  }


  if (nearestd1<nearestd2) {
    if (nearestd1<100000) {
      *nearestdistance = nearestd1;
      *nearestbearing = nearestb1;
      *foundarea = -1;
    }
  } else {
    if (nearestd2<100000) {
      *nearestdistance = nearestd2;
      *nearestbearing = nearestb2;
      *foundcircle = -1;
    }
  }
  return;
}



/////////////


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


bool line_rect_intersection (const double x1,
			     const double y1,
			     const double dx,
			     const double dy,
			     rectObj *bounds) {
  double u;

  // bottom line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->minx, bounds->miny,
			     bounds->maxx, bounds->miny,
			     &u)) return true;

  // left line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->minx, bounds->miny,
			     bounds->minx, bounds->maxy,
			     &u)) return true;

  // top line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->minx, bounds->maxy,
			     bounds->maxx, bounds->maxy,
			     &u)) return true;

  // right line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->maxx, bounds->miny,
			     bounds->maxx, bounds->maxy,
			     &u)) return true;
  return false;
}


void ScanAirspaceLine(double *lats, double *lons, double *heights,
		      int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X])
{

  int i,j;
  unsigned int k;
  double latitude, longitude, Dist;
  double x1 = lons[0];
  double dx = lons[AIRSPACE_SCANSIZE_X-1]-x1;
  double y1 = lats[0];
  double dy = lats[AIRSPACE_SCANSIZE_X-1]-y1;
  double h_min = heights[0];
  double h_max = heights[AIRSPACE_SCANSIZE_H-1];

  rectObj lineRect;

  lineRect.minx = min(x1, x1+dx);
  lineRect.maxx = max(x1, x1+dx);
  lineRect.miny = min(y1, y1+dy);
  lineRect.maxy = max(y1, y1+dy);

  for(k=0;k<NumberOfAirspaceCircles;k++) {

    // ignore if outside scan height
    if (!((h_max<=AirspaceCircle[k].Base.Altitude)||
	  (h_min>=AirspaceCircle[k].Top.Altitude))) {

      // ignore if scan line doesn't intersect bounds
      if (msRectOverlap(&lineRect, &AirspaceCircle[k].bounds) &&
	  line_rect_intersection (x1, y1, dx, dy,
				  &AirspaceCircle[k].bounds)) {

	for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
	  latitude = lats[i];
	  longitude = lons[i];
	  if ((latitude> AirspaceCircle[k].bounds.miny)&&
	      (latitude< AirspaceCircle[k].bounds.maxy)&&
	      CheckInsideLongitude(longitude,
				   AirspaceCircle[k].bounds.minx,
				   AirspaceCircle[k].bounds.maxx)) {

	    DistanceBearing(latitude,longitude,
			    AirspaceCircle[k].Latitude,
			    AirspaceCircle[k].Longitude, &Dist, NULL);
	    Dist -= AirspaceCircle[k].Radius;

	    if(Dist < 0) {
	      for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
		if ((heights[j]>AirspaceCircle[k].Base.Altitude)&&
		    (heights[j]<AirspaceCircle[k].Top.Altitude)) {
		  airspacetype[j][i] = AirspaceCircle[k].Type;
		} // inside height
	      } // finished scanning height
	    } // inside
	  } // in bound
	} // finished scanning range
      } // line intersects
    } // within height
  } // finished scanning circles

  for(k=0;k<NumberOfAirspaceAreas;k++) {

    // ignore if outside scan height
    if (!((h_max<=AirspaceArea[k].Base.Altitude)||
	  (h_min>=AirspaceArea[k].Top.Altitude))) {

      // ignore if scan line doesn't intersect bounds
      if (msRectOverlap(&lineRect, &AirspaceArea[k].bounds) &&
	  line_rect_intersection (x1, y1, dx, dy,
				  &AirspaceArea[k].bounds)) {

	for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
	  latitude = lats[i];
	  longitude = lons[i];

	  if ((latitude> AirspaceArea[k].bounds.miny)&&
	      (latitude< AirspaceArea[k].bounds.maxy)&&
	      CheckInsideLongitude(longitude,
				   AirspaceArea[k].bounds.minx,
				   AirspaceArea[k].bounds.maxx)) {
	    AIRSPACE_POINT thispoint;
	    thispoint.Longitude = longitude;
	    thispoint.Latitude = latitude;

	    CheckAirspacePoint(AirspaceArea[k].FirstPoint);

	    if (wn_PnPoly(thispoint,
			  &AirspacePoint[AirspaceArea[k].FirstPoint],
			  AirspaceArea[k].NumPoints-1) != 0) {
	      for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
		if ((heights[j]>AirspaceArea[k].Base.Altitude)&&
		    (heights[j]<AirspaceArea[k].Top.Altitude)) {
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


#if 0
// old...
void ScanAirspaceLine_old(double *lats, double *lons, double *heights,
		      int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X])
{

  int i,j;
  unsigned int k;
  double latitude, longitude, height, Dist;

  for(k=0;k<NumberOfAirspaceCircles;k++) {
    for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
      latitude = lats[i];
      longitude = lons[i];
      if ((latitude> AirspaceCircle[k].bounds.miny)&&
	  (latitude< AirspaceCircle[k].bounds.maxy)&&
          CheckInsideLongitude(longitude,
                               AirspaceCircle[k].bounds.minx,
                               AirspaceCircle[k].bounds.maxx)) {

        DistanceBearing(latitude,longitude,
                        AirspaceCircle[k].Latitude,
                        AirspaceCircle[k].Longitude, &Dist, NULL);
	Dist -= AirspaceCircle[k].Radius;

	if(Dist < 0) {
	  for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
	    height = heights[j];
	    if ((height>AirspaceCircle[k].Base.Altitude)&&
		(height<AirspaceCircle[k].Top.Altitude)) {
	      airspacetype[j][i] = AirspaceCircle[k].Type;
	    } // inside height
	  } // finished scanning height
	} // inside
      } // in bound
    } // finished scanning range
  } // finished scanning circles

  for(k=0;k<NumberOfAirspaceAreas;k++) {
    for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
      latitude = lats[i];
      longitude = lons[i];

      if ((latitude> AirspaceArea[k].bounds.miny)&&
	  (latitude< AirspaceArea[k].bounds.maxy)&&
          CheckInsideLongitude(longitude,
                               AirspaceArea[k].bounds.minx,
                               AirspaceArea[k].bounds.maxx)) {
	AIRSPACE_POINT thispoint;
	thispoint.Longitude = longitude;
	thispoint.Latitude = latitude;

        CheckAirspacePoint(AirspaceArea[k].FirstPoint);

	if (wn_PnPoly(thispoint,
		      &AirspacePoint[AirspaceArea[k].FirstPoint],
		      AirspaceArea[k].NumPoints-1) != 0) {
	  for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
	    height = heights[j];
	    if ((height>AirspaceArea[k].Base.Altitude)&&
		(height<AirspaceArea[k].Top.Altitude)) {
	      airspacetype[j][i] = AirspaceArea[k].Type;
	    } // inside height
	  } // finished scanning height
	} // inside
      } // in bound
    } // finished scanning range
  } // finished scanning areas

}
#endif
