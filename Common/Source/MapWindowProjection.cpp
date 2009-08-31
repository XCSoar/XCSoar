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

#include "MapWindow.h"
#include "Protection.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "InfoBoxLayout.h"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "InputEvents.h"
#include "SettingsUser.hpp"
#include "UtilsProfile.hpp"
#include <stdlib.h>

int GliderScreenPosition = 20; // 20% from bottom
DisplayOrientation_t DisplayOrientation = TRACKUP;

MapWindowProjection::MapWindowProjection():
  GliderCenter(false),
  PanLatitude ( 0.0),
  PanLongitude ( 0.0),
  DisplayAngle ( 0.0),
  RequestMapScale(5),
  MapScale(5),
  MapScaleOverDistanceModify(5/DISTANCEMODIFY),
  ResMapScaleOverDistanceModify ( 0.0),
  DisplayAircraftAngle ( 0.0),
  ScaleListCount ( 0),
  EnablePan ( false),
  TargetPan ( false),
  TargetPanIndex ( 0),
  TargetZoomDistance ( 500.0)
{

}

#include "WayPoint.hpp"


void MapWindowProjection::InitialiseScaleList(void) {
  ScaleListCount = propGetScaleList(ScaleList, sizeof(ScaleList)/sizeof(ScaleList[0]));
  RequestMapScale = LimitMapScale(RequestMapScale);
}

bool MapWindowProjection::WaypointInRange(int i) {
  return ((WayPointList[i].Zoom >= MapScale*10)
          || (WayPointList[i].Zoom == 0))
    && (MapScale <= 10);
}


bool MapWindowProjection::PointInRect(const double &lon,
			    const double &lat,
                            const rectObj &bounds) {
  if ((lon> bounds.minx) &&
      (lon< bounds.maxx) &&
      (lat> bounds.miny) &&
      (lat< bounds.maxy))
    return true;
  else
    return false;
}


bool MapWindowProjection::PointVisible(const double &lon,
				       const double &lat) {
  if ((lon> screenbounds_latlon.minx) &&
      (lon< screenbounds_latlon.maxx) &&
      (lat> screenbounds_latlon.miny) &&
      (lat< screenbounds_latlon.maxy))
    return true;
  else
    return false;
}


bool MapWindowProjection::PointVisible(const POINT &P)
{
  if(( P.x >= MapRect.left )
     &&
     ( P.x <= MapRect.right )
     &&
     ( P.y >= MapRect.top  )
     &&
     ( P.y <= MapRect.bottom  )
     )
    return TRUE;
  else
    return FALSE;
}


rectObj MapWindowProjection::CalculateScreenBounds(double scale) {
  // compute lat lon extents of visible screen
  rectObj sb;

  if (scale>= 1.0) {
    POINT screen_center;
    LatLon2Screen(PanLongitude,
                  PanLatitude,
                  screen_center);

    sb.minx = sb.maxx = PanLongitude;
    sb.miny = sb.maxy = PanLatitude;

    int dx, dy;
    unsigned int maxsc=0;
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));

    for (int i=0; i<10; i++) {
      double ang = i*360.0/10;
      POINT p;
      double X, Y;
      p.x = screen_center.x + iround(fastcosine(ang)*maxsc*scale);
      p.y = screen_center.y + iround(fastsine(ang)*maxsc*scale);
      Screen2LatLon(p.x, p.y, X, Y);
      sb.minx = min(X, sb.minx);
      sb.miny = min(Y, sb.miny);
      sb.maxx = max(X, sb.maxx);
      sb.maxy = max(Y, sb.maxy);
    }

  } else {

    double xmin, xmax, ymin, ymax;
    int x, y;
    double X, Y;

    x = MapRect.left;
    y = MapRect.top;
    Screen2LatLon(x, y, X, Y);
    xmin = X; xmax = X;
    ymin = Y; ymax = Y;

    x = MapRect.right;
    y = MapRect.top;
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);

    x = MapRect.right;
    y = MapRect.bottom;
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);

    x = MapRect.left;
    y = MapRect.bottom;
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);


    sb.minx = xmin;
    sb.maxx = xmax;
    sb.miny = ymin;
    sb.maxy = ymax;

  }

  return sb;
}

////////////////////////////////////////////////////////////////////
// RETURNS Longitude, Latitude!

void MapWindowProjection::OrigScreen2LatLon(const int &x, const int &y,
					    double &X, double &Y)
{
  int sx = x;
  int sy = y;
  irotate(sx, sy, DisplayAngle);
  Y= PanLatitude  - sy*InvDrawScale;
  X= PanLongitude + sx*invfastcosine(Y)*InvDrawScale;
}


void MapWindowProjection::Screen2LatLon(const int &x,
					const int &y,
					double &X, double &Y)
{
  int sx = x-(int)Orig_Screen.x;
  int sy = y-(int)Orig_Screen.y;
  irotate(sx, sy, DisplayAngle);
  Y= PanLatitude  - sy*InvDrawScale;
  X= PanLongitude + sx*invfastcosine(Y)*InvDrawScale;
}

void MapWindowProjection::LatLon2Screen(const double &lon,
					const double &lat,
					POINT &sc) {

  int Y = Real2Int((PanLatitude-lat)*DrawScale);
  int X = Real2Int((PanLongitude-lon)*fastcosine(lat)*DrawScale);

  irotate(X, Y, DisplayAngle);

  sc.x = Orig_Screen.x - X;
  sc.y = Orig_Screen.y + Y;
}

// This one is optimised for long polygons
void MapWindowProjection::LatLon2Screen(pointObj *ptin,
					POINT *ptout,
					const int n,
					const int skip) {
  static double lastangle = -1;
  static int cost=1024, sint=0;
  const double mDisplayAngle = DisplayAngle;

  if(mDisplayAngle != lastangle) {
    lastangle = mDisplayAngle;
    int deg = DEG_TO_INT(AngleLimit360(mDisplayAngle));
    cost = ICOSTABLE[deg];
    sint = ISINETABLE[deg];
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const double mDrawScale = DrawScale;
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;
  pointObj* p = ptin;
  const pointObj* ptend = ptin+n;

  while (p<ptend) {
    int Y = Real2Int((mPanLatitude-p->y)*mDrawScale);
    int X = Real2Int((mPanLongitude-p->x)*fastcosine(p->y)*mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}

////////////////////////////////////////////////////////////////////


void MapWindowProjection::CalculateOrientationNormal(void) {
  double trackbearing = DrawInfo.TrackBearing;

  if( (DisplayOrientation == NORTHUP)
      ||
      ((DisplayOrientation == NORTHTRACK)
       &&(DisplayMode != dmCircling))
      ||
      (
       ((DisplayOrientation == NORTHCIRCLE)
        ||(DisplayOrientation==TRACKCIRCLE))
       && (DisplayMode == dmCircling) )
      ) {
    GliderCenter = true;

    if (DisplayOrientation == TRACKCIRCLE) {
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle = trackbearing-DisplayAngle;
    } else {
      DisplayAngle = 0.0;
      DisplayAircraftAngle = trackbearing;
    }
  } else {
    // normal, glider forward
    GliderCenter = false;
    DisplayAngle = trackbearing;
    DisplayAircraftAngle = 0.0;
  }
  DisplayAngle = AngleLimit360(DisplayAngle);
  DisplayAircraftAngle = AngleLimit360(DisplayAircraftAngle);
}


void MapWindowProjection::CalculateOrientationTargetPan(void) {
  // Target pan mode, show track up when looking at current task point,
  // otherwise north up.  If circling, orient towards target.

  GliderCenter = true;
  if ((ActiveWayPoint==TargetPanIndex)
      &&(DisplayOrientation != NORTHUP)
      &&(DisplayOrientation != NORTHTRACK)
      )    {
    if (DisplayMode == dmCircling) {
      // target-up
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle =
        DrawInfo.TrackBearing-DisplayAngle;
    } else {
      // track up
      DisplayAngle = DrawInfo.TrackBearing;
      DisplayAircraftAngle = 0.0;
    }
  } else {
    // North up
    DisplayAngle = 0.0;
    DisplayAircraftAngle = DrawInfo.TrackBearing;
  }
}


void MapWindowProjection::CalculateOrigin(const RECT rc, POINT *Orig)
{

  mutexTaskData.Lock();
  if (TargetPan) {
    CalculateOrientationTargetPan();
  } else {
    CalculateOrientationNormal();
  }
  mutexTaskData.Unlock();

  if (GliderCenter || EnablePan) {
    Orig->x = (rc.left + rc.right)/2;
    Orig->y = (rc.bottom + rc.top)/2;
  } else {
    Orig->x = (rc.left + rc.right)/2;
    Orig->y = ((rc.top - rc.bottom )*GliderScreenPosition/100)+rc.bottom;
  }
}


void MapWindow::Event_Pan(int vswitch) {
  //  static bool oldfullscreen = 0;  never assigned!
  bool oldPan = EnablePan;
  if (vswitch == -2) { // superpan, toggles fullscreen also

    if (!EnablePan) {
      StoreRestoreFullscreen(true);
    } else {
      StoreRestoreFullscreen(false);
    }
    // new mode
    EnablePan = !EnablePan;
    if (EnablePan) { // pan now on, so go fullscreen
      askFullScreen = true;
    }

  } else if (vswitch == -1) {
    EnablePan = !EnablePan;
  } else {
    EnablePan = (vswitch != 0); // 0 off, 1 on
  }

  if (EnablePan != oldPan) {
    if (EnablePan) {
      PanLongitude = DrawInfo.Longitude;
      PanLatitude = DrawInfo.Latitude;
      InputEvents::setMode(TEXT("pan"));
    } else
      InputEvents::setMode(TEXT("default"));
  }
  RefreshMap();
}


double MapWindowProjection::GetApproxScreenRange() {
  return (MapScale * max(MapRectBig.right-MapRectBig.left,
                         MapRectBig.bottom-MapRectBig.top))
    *1000.0/GetMapResolutionFactor();
}


int MapWindowProjection::GetMapResolutionFactor(void) {
  return IBLSCALE(30);
}


/////


double MapWindowProjection::LimitMapScale(double value) {

  double minreasonable;

  minreasonable = 0.05;

  if (AutoZoom && DisplayMode != dmCircling) {
    if (AATEnabled && (ActiveWayPoint>0)) {
      minreasonable = 0.88;
    } else {
      minreasonable = 0.44;
    }
  }

  if (ScaleListCount>0) {
    return FindMapScale(max(minreasonable,min(160.0,value)));
  } else {
    return max(minreasonable,min(160.0,value));
  }
}


double MapWindowProjection::StepMapScale(int Step){
  static int nslow=0;
  if (abs(Step)>=4) {
    nslow++;
    //    if (nslow %2 == 0) {
    ScaleCurrent += Step/4;
    //    }
  } else {
    ScaleCurrent += Step;
  }
  ScaleCurrent = max(0,min(ScaleListCount-1, ScaleCurrent));
  return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
         /(IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right)));
}

double MapWindowProjection::FindMapScale(double Value){

  int    i;
  double BestFit = 99999;
  int    BestFitIdx=-1;
  double DesiredScale =
    (Value*IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right))/GetMapResolutionFactor();

  for (i=0; i<ScaleListCount; i++){
    double err = fabs(DesiredScale - ScaleList[i])/DesiredScale;
    if (err < BestFit){
      BestFit = err;
      BestFitIdx = i;
    }
  }

  if (BestFitIdx != -1){
    ScaleCurrent = BestFitIdx;
    return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
           /IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right));
  }
  return(Value);
}



void MapWindowProjection::ModifyMapScale(void) {
  // limit zoomed in so doesn't reach silly levels
  RequestMapScale = LimitMapScale(RequestMapScale); // FIX VENTA remove limit
  MapScaleOverDistanceModify = RequestMapScale/DISTANCEMODIFY;
  ResMapScaleOverDistanceModify =
    GetMapResolutionFactor()/MapScaleOverDistanceModify;
  DrawScale = MapScaleOverDistanceModify;
  DrawScale = DrawScale/111194;
  DrawScale = GetMapResolutionFactor()/DrawScale;
  InvDrawScale = 1.0/DrawScale;
  MapScale = RequestMapScale;
}


void MapWindowProjection::UpdateMapScale()
{
  static int AutoMapScaleWaypointIndex = -1;
  static double StartingAutoMapScale=0.0;
  double AutoZoomFactor;

  bool user_asked_for_change = false;

  mutexTaskData.Lock();
  bool my_target_pan = TargetPan;
  mutexTaskData.Unlock();

  // if there is user intervention in the scale
  if(MapScale != RequestMapScale) {
    ModifyMapScale();
    user_asked_for_change = true;
  }

  double wpd;
  if (my_target_pan) {
    wpd = TargetZoomDistance;
  } else {
    wpd = DerivedDrawInfo.ZoomDistance;
  }
  if (my_target_pan) {
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen
    RequestMapScale = LimitMapScale(wpd
                                    *DISTANCEMODIFY/ 4.0);
    ModifyMapScale();
    return;
  }

  if (AutoZoom) {
    if(wpd > 0)
      {

	if(
	   (((DisplayOrientation == NORTHTRACK)
	     &&(DisplayMode != dmCircling))
	    ||(DisplayOrientation == NORTHUP)
	    ||
	    (((DisplayOrientation == NORTHCIRCLE)
	      || (DisplayOrientation == TRACKCIRCLE))
	     && (DisplayMode == dmCircling) ))
	   && !my_target_pan
	   )
	  {
	    AutoZoomFactor = 2.5;
	  }
	else
	  {
	    AutoZoomFactor = 4;
	  }

	if(
	   (wpd < ( AutoZoomFactor * MapScaleOverDistanceModify))
	   ||
	   (StartingAutoMapScale==0.0))
	  {
	    // waypoint is too close, so zoom in
	    // OR just turned waypoint

	    // this is the first time this waypoint has gotten close,
	    // so save original map scale

	    if (StartingAutoMapScale==0.0) {
	      StartingAutoMapScale = MapScale;
	    }

	    // set scale exactly so that waypoint distance is the zoom factor
	    // across the screen
	    RequestMapScale = LimitMapScale(wpd
					    *DISTANCEMODIFY/ AutoZoomFactor);
	    ModifyMapScale();

	  } else {

	  if (user_asked_for_change) {

	    // user asked for a zoom change and it was achieved, so
	    // reset starting map scale
	    ////?TODO enhancement: for frank          StartingAutoMapScale = MapScale;
	  }

	}
      }
  } else {

    // reset starting map scale for auto zoom if momentarily switch
    // off autozoom
    //    StartingAutoMapScale = RequestMapScale;
  }

  if (my_target_pan) {
    return;
  }

  mutexTaskData.Lock();  // protect from extrnal task changes
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    // if we aren't looking at a waypoint, see if we are now
    if (AutoMapScaleWaypointIndex == -1) {
      if (ValidTaskPoint(ActiveWayPoint)) {
	AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;
      }
    }

    // if there is an active waypoint
    if (ValidTaskPoint(ActiveWayPoint)) {

      // if the current zoom focused waypoint has changed...
      if (AutoMapScaleWaypointIndex != Task[ActiveWayPoint].Index) {
	AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;

	// zoom back out to where we were before
	if (StartingAutoMapScale> 0.0) {
	  RequestMapScale = StartingAutoMapScale;
	}

	// reset search for new starting zoom level
	StartingAutoMapScale = 0.0;
      }

    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       mutexTaskData.Unlock();
     }

}

#include "Screen/Graphics.hpp"

void MapWindowProjection::DrawGreatCircle(Canvas &canvas,
					  double startLon, double startLat,
					  double targetLon, double targetLat,
					  const RECT rc) {

#if OLD_GREAT_CIRCLE
  // TODO accuracy: this is actually wrong, it should recalculate the
  // bearing each step
  double distance=0;
  double distanceTotal=0;
  double Bearing;

  DistanceBearing(startLat,
                  startLon,
                  targetLat,
                  targetLon,
                  &distanceTotal,
                  &Bearing);

  distance = distanceTotal;

  if (distanceTotal==0.0) {
    return;
  }

  double d_distance = max(5000.0,distanceTotal/10);

  canvas.select(MapGfx.hpBearing);

  POINT StartP;
  POINT EndP;
  LatLon2Screen(startLon,
		startLat,
		StartP);
  LatLon2Screen(targetLon,
		targetLat,
		EndP);

  if (d_distance>distanceTotal) {
    canvas.clipped_line(StartP, EndP, rc);
  } else {

    for (int i=0; i<= 10; i++) {

      double tlat1, tlon1;

      FindLatitudeLongitude(startLat,
                            startLon,
                            Bearing,
                            min(distance,d_distance),
                            &tlat1,
                            &tlon1);

      DistanceBearing(tlat1,
                      tlon1,
                      targetLat,
                      targetLon,
                      &distance,
                      &Bearing);

      LatLon2Screen(tlon1,
                    tlat1,
                    EndP);

      canvas.clipped_line(StartP, EndP, rc);

      StartP.x = EndP.x;
      StartP.y = EndP.y;

      startLat = tlat1;
      startLon = tlon1;

    }
  }
#else
  // Simple and this should work for PNA with display bug

  canvas.select(MapGfx.hpBearing);
  POINT pt[2];
  LatLon2Screen(startLon,
                startLat,
                pt[0]);
  LatLon2Screen(targetLon,
                targetLat,
                pt[1]);
  canvas.clipped_polygon(pt, 2, rc, false);

#endif
}
