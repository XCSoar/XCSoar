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

#include "MapWindowProjection.hpp"
#include "Protection.hpp"
#include "MapWindow.h"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "InfoBoxLayout.h"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "InputEvents.h"
#include "UtilsProfile.hpp"
#include "options.h" /* for IBLSCALE() */
#include <stdlib.h>
#include "WayPoint.hpp"
#include "WayPointList.hpp"
#include "Components.hpp"
#include <math.h>


MapWindowProjection::MapWindowProjection():
  _origin_centered(false),
  DisplayAngle ( 0.0),
  _RequestedMapScale(5),
  MapScale(5),
  _scale_meters_to_screen ( 0.0),
  DisplayAircraftAngle ( 0.0),
  ScaleListCount ( 0),
  smart_range_active(1.0),
  DisplayMode(dmCruise)
{
  PanLocation.Latitude = 0.0;
  PanLocation.Longitude = 0.0;
}


void 
MapWindowProjection::InitialiseScaleList
(const SETTINGS_MAP &settings_map) 
{
  ScaleListCount = propGetScaleList(ScaleList, 
				    sizeof(ScaleList)/sizeof(ScaleList[0]));
  _RequestedMapScale = LimitMapScale(_RequestedMapScale, settings_map);
}

bool MapWindowProjection::WaypointInScaleFilter(int i) const
{
  const WAYPOINT &way_point = way_points.get(i);

  return ((way_point.Zoom >= MapScale*10) || (way_point.Zoom == 0))
    && (MapScale <= 10);
}


bool MapWindowProjection::PointInRect(const double &x,
				      const double &y,
				      const rectObj &bounds) const
{
  if ((x> bounds.minx) &&
      (x< bounds.maxx) &&
      (y> bounds.miny) &&
      (y< bounds.maxy))
    return true;
  else
    return false;
}


bool 
MapWindowProjection::LonLatVisible(const GEOPOINT &loc) const
{
  if ((loc.Longitude> screenbounds_latlon.minx) &&
      (loc.Longitude< screenbounds_latlon.maxx) &&
      (loc.Latitude> screenbounds_latlon.miny) &&
      (loc.Latitude< screenbounds_latlon.maxy))
    return true;
  else
    return false;
}

bool 
MapWindowProjection::LonLat2ScreenIfVisible(const GEOPOINT &loc, 
					    POINT *sc) const
{
  if (LonLatVisible(loc)) {
    LonLat2Screen(loc, *sc);
    return PointVisible(*sc);
  } else {
    return false;
  }
}


bool
MapWindowProjection::PointVisible(const POINT &P) const
{
  if(( P.x >= MapRect.left )
     &&
     ( P.x <= MapRect.right )
     &&
     ( P.y >= MapRect.top  )
     &&
     ( P.y <= MapRect.bottom  )
     )
    return true;
  else
    return false;
}


rectObj
MapWindowProjection::CalculateScreenBounds(double scale) const
{
  // compute lat lon extents of visible screen
  rectObj sb;

  if (scale>= 1.0) {
    POINT screen_center;
    LonLat2Screen(PanLocation,screen_center);

    sb.minx = sb.maxx = PanLocation.Longitude;
    sb.miny = sb.maxy = PanLocation.Latitude;

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
      GEOPOINT g;
      p.x = screen_center.x + iround(fastcosine(ang)*maxsc*scale);
      p.y = screen_center.y + iround(fastsine(ang)*maxsc*scale);
      Screen2LonLat(p.x, p.y, g);
      sb.minx = min(g.Longitude, sb.minx);
      sb.miny = min(g.Latitude, sb.miny);
      sb.maxx = max(g.Longitude, sb.maxx);
      sb.maxy = max(g.Latitude, sb.maxy);
    }

  } else {

    double xmin, xmax, ymin, ymax;
    int x, y;
    GEOPOINT g;

    x = MapRect.left;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = g.Longitude; xmax = g.Longitude;
    ymin = g.Latitude; ymax = g.Latitude;

    x = MapRect.right;
    y = MapRect.top;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, g.Longitude); xmax = max(xmax, g.Longitude);
    ymin = min(ymin, g.Latitude); ymax = max(ymax, g.Latitude);

    x = MapRect.right;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, g.Longitude); xmax = max(xmax, g.Longitude);
    ymin = min(ymin, g.Latitude); ymax = max(ymax, g.Latitude);

    x = MapRect.left;
    y = MapRect.bottom;
    Screen2LonLat(x, y, g);
    xmin = min(xmin, g.Longitude); xmax = max(xmax, g.Longitude);
    ymin = min(ymin, g.Latitude); ymax = max(ymax, g.Latitude);

    sb.minx = xmin;
    sb.maxx = xmax;
    sb.miny = ymin;
    sb.maxy = ymax;

  }

  return sb;
}

////////////////////////////////////////////////////////////////////
// RETURNS Longitude, Latitude!


void MapWindowProjection::Screen2LonLat(const int &x,
					const int &y,
					GEOPOINT &g) const
{
  int sx = x-(int)Orig_Screen.x;
  int sy = y-(int)Orig_Screen.y;
  irotate(sx, sy, DisplayAngle);
  g.Latitude= PanLocation.Latitude  - sy*InvDrawScale;
  g.Longitude= PanLocation.Longitude + sx*invfastcosine(g.Latitude)*InvDrawScale;
}

void MapWindowProjection::LonLat2Screen(const GEOPOINT &g,
					POINT &sc) const
{
  int Y = Real2Int((PanLocation.Latitude-g.Latitude)*DrawScale);
  int X = Real2Int((PanLocation.Longitude-g.Longitude)*fastcosine(g.Latitude)*DrawScale);

  irotate(X, Y, DisplayAngle);

  sc.x = Orig_Screen.x - X;
  sc.y = Orig_Screen.y + Y;
}

// This one is optimised for long polygons
void MapWindowProjection::LonLat2Screen(const pointObj* const ptin,
					POINT *ptout,
					const int n,
					const int skip) const
{
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
  const double mPanLongitude = PanLocation.Longitude;
  const double mPanLatitude = PanLocation.Latitude;
  pointObj const * p = ptin;
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


void 
MapWindowProjection::CalculateOrientationNormal
(const NMEA_INFO &DrawInfo,
 const DERIVED_INFO &DerivedDrawInfo,
 const SETTINGS_MAP &settings) 

{
  double trackbearing = DrawInfo.TrackBearing;

  if( (settings.DisplayOrientation == NORTHUP)
      ||
      ((settings.DisplayOrientation == NORTHTRACK)
       &&(DisplayMode != dmCircling))
      ||
      (
       ((settings.DisplayOrientation == NORTHCIRCLE)
        ||(settings.DisplayOrientation==TRACKCIRCLE))
       && (DisplayMode == dmCircling) )
      ) {
    _origin_centered = true;

    if (settings.DisplayOrientation == TRACKCIRCLE) {
      DisplayAngle = DerivedDrawInfo.WaypointBearing;
      DisplayAircraftAngle = trackbearing-DisplayAngle;
    } else {
      DisplayAngle = 0.0;
      DisplayAircraftAngle = trackbearing;
    }
  } else {
    // normal, glider forward
    _origin_centered = false;
    DisplayAngle = trackbearing;
    DisplayAircraftAngle = 0.0;
  }
  DisplayAngle = AngleLimit360(DisplayAngle);
  DisplayAircraftAngle = AngleLimit360(DisplayAircraftAngle);
}


void 
MapWindowProjection::CalculateOrientationTargetPan
(const NMEA_INFO &DrawInfo,
 const DERIVED_INFO &DerivedDrawInfo,
 const SETTINGS_MAP &settings) 

{
  // Target pan mode, show track up when looking at current task point,
  // otherwise north up.  If circling, orient towards target.

  _origin_centered = true;
  if (((int)task.getActiveIndex()==settings.TargetPanIndex)
      &&(settings.DisplayOrientation != NORTHUP)
      &&(settings.DisplayOrientation != NORTHTRACK)
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


void 
MapWindowProjection::CalculateOrigin
(const RECT rc,
 const NMEA_INFO &DrawInfo,
 const DERIVED_INFO &DerivedDrawInfo,
 const SETTINGS_COMPUTER &settings_computer,
 const SETTINGS_MAP &settings_map)
{

  if (settings_map.TargetPan) {
    CalculateOrientationTargetPan(DrawInfo, DerivedDrawInfo, settings_map);
  } else {
    CalculateOrientationNormal(DrawInfo, DerivedDrawInfo, settings_map);
  }

  if (_origin_centered || settings_map.EnablePan) {
    Orig_Screen.x = (rc.left + rc.right)/2;
    Orig_Screen.y = (rc.bottom + rc.top)/2;
  } else {
    Orig_Screen.x = (rc.left + rc.right)/2;
    Orig_Screen.y = ((rc.top - rc.bottom )*
		     settings_map.GliderScreenPosition/100)+rc.bottom;
  }

  //
  if (settings_map.EnablePan) {
    PanLocation = settings_map.PanLocation;
  } else {

    if (IsOriginCentered()
        && DerivedDrawInfo.Circling
        && (settings_computer.EnableThermalLocator==2)) {

      if (DerivedDrawInfo.ThermalEstimate_R>0) {
        PanLocation = DerivedDrawInfo.ThermalEstimate_Location;
        // TODO enhancement: only pan if distance of center to
        // aircraft is smaller than one third screen width

        POINT screen;
        LonLat2Screen(PanLocation, screen);

        LonLat2Screen(DrawInfo.Location, Orig_Aircraft);

        if ((fabs((double)Orig_Aircraft.x-screen.x)<(rc.right-rc.left)/3)
            && (fabs((double)Orig_Aircraft.y-screen.y)<(rc.bottom-rc.top)/3)) {

        } else {
          // out of bounds, center on aircraft
          PanLocation = DrawInfo.Location;
        }
      } else {
        // out of bounds, center on aircraft
        PanLocation = DrawInfo.Location;
      }
    } else {
      // Pan is off
      PanLocation = DrawInfo.Location;
    }
  }

  LonLat2Screen(DrawInfo.Location,
                Orig_Aircraft);

  screenbounds_latlon = CalculateScreenBounds(0.0);
}




double
MapWindowProjection::GetScreenDistanceMeters() const
{
  return DistancePixelsToMeters(max(MapRectBig.right-MapRectBig.left,
				    MapRectBig.bottom-MapRectBig.top));
}

int
MapWindowProjection::GetMapResolutionFactor(void) const
{
  return IBLSCALE(30);
}


/////


double MapWindowProjection::LimitMapScale(double value,
					  const SETTINGS_MAP& settings_map) {

  double minreasonable;

  minreasonable = 0.05;

  if (settings_map.AutoZoom && DisplayMode != dmCircling) {
    if (AATEnabled && (task.getActiveIndex()>0)) {
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
  if (abs(Step)>=4) {
    ScaleCurrent += Step/4;
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
           /IBLSCALE(/*Appearance.DefaultMapWidth*/MapRect.right-MapRect.left));
  }
  return(Value);
}



void MapWindowProjection::ModifyMapScale
(const SETTINGS_MAP &settings_map) 
{
  // limit zoomed in so doesn't reach silly levels
  _RequestedMapScale = 
    LimitMapScale(_RequestedMapScale, settings_map);
  MapScale = _RequestedMapScale;

  _scale_meters_to_screen =
    GetMapResolutionFactor()*DISTANCEMODIFY/MapScale;
  DrawScale = 111194*_scale_meters_to_screen;
  InvDrawScale = 1.0/DrawScale;
}


void MapWindowProjection::UpdateMapScale(const NMEA_INFO &DrawInfo,
					 const DERIVED_INFO &DerivedDrawInfo,
					 const SETTINGS_MAP &settings_map)
{
  static int AutoMapScaleWaypointIndex = -1;
  static double StartingAutoMapScale=0.0;
  double AutoZoomFactor;
  static DisplayMode_t DisplayModeLast = DisplayMode;

  // if there is user intervention in the scale
  if (settings_map.MapScale>0) {
    double ext_mapscale = LimitMapScale(settings_map.MapScale, settings_map);
    if ((fabs(_RequestedMapScale-ext_mapscale)>0.05) && 
	(ext_mapscale>0.0) && (DisplayMode==DisplayModeLast)) {
      _RequestedMapScale = ext_mapscale;
    }
  }
  if(MapScale != _RequestedMapScale) {
    ModifyMapScale(settings_map);
  }
  DisplayModeLast = DisplayMode;

  double wpd;
  if (settings_map.TargetPan) {
    wpd = settings_map.TargetZoomDistance;
  } else {
    wpd = DerivedDrawInfo.ZoomDistance;
  }
  if (settings_map.TargetPan) {
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen
    _RequestedMapScale = LimitMapScale(wpd*DISTANCEMODIFY/4.0, settings_map);
    ModifyMapScale(settings_map);
    return;
  }

  if (settings_map.AutoZoom && wpd>0) {
    if((((settings_map.DisplayOrientation == NORTHTRACK)
	 &&(DisplayMode != dmCircling))
	||(settings_map.DisplayOrientation == NORTHUP)
	||
	(((settings_map.DisplayOrientation == NORTHCIRCLE)
	  || (settings_map.DisplayOrientation == TRACKCIRCLE))
	 && (DisplayMode == dmCircling) ))
       && !settings_map.TargetPan
       ) {
      AutoZoomFactor = 2.5;
    } else {
      AutoZoomFactor = 4;
    }
    
    if((wpd < ( AutoZoomFactor * MapScale/DISTANCEMODIFY))
       || (StartingAutoMapScale==0.0)) {
      // waypoint is too close, so zoom in
      // OR just turned waypoint
      
      // this is the first time this waypoint has gotten close,
      // so save original map scale
      
      if (StartingAutoMapScale==0.0) {
	StartingAutoMapScale = MapScale;
      }
      
      // set scale exactly so that waypoint distance is the zoom factor
      // across the screen
      _RequestedMapScale = 
	LimitMapScale(wpd*DISTANCEMODIFY/ AutoZoomFactor, settings_map);
      ModifyMapScale(settings_map);
      
    }
  }

  mutexTaskData.Lock();  // protect from extrnal task changes
  // if we aren't looking at a waypoint, see if we are now
  if (AutoMapScaleWaypointIndex == -1) {
    if (task.Valid()) {
      AutoMapScaleWaypointIndex = task_points[task.getActiveIndex()].Index;
    }
  }
  // if there is an active waypoint
  if (task.Valid()) {
    // if the current zoom focused waypoint has changed...
    if (AutoMapScaleWaypointIndex != task_points[task.getActiveIndex()].Index) {
      AutoMapScaleWaypointIndex = task_points[task.getActiveIndex()].Index;
      
      // zoom back out to where we were before
      if (StartingAutoMapScale> 0.0) {
	_RequestedMapScale = StartingAutoMapScale;
	ModifyMapScale(settings_map);
      }
      
      // reset search for new starting zoom level
      StartingAutoMapScale = 0.0;
    }
    
  }
  mutexTaskData.Unlock();
}


void MapWindowProjection::ExchangeBlackboard(const NMEA_INFO &nmea_info,
					     const DERIVED_INFO &derived_info,
					     const SETTINGS_MAP &settings_map) 
{
  UpdateMapScale(nmea_info, derived_info, settings_map); 
  // done here to avoid double latency due to locks
}


#include "Screen/Graphics.hpp"

void MapWindowProjection::DrawGreatCircle(Canvas &canvas,
					  const GEOPOINT &loc_start, 
                                          const GEOPOINT &loc_end) {

#ifdef OLD_GREAT_CIRCLE
  // TODO accuracy: this is actually wrong, it should recalculate the
  // bearing each step
  double distance=0;
  double distanceTotal=0;
  double Bearing;

  DistanceBearing(loc_start,
                  loc_end,
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
  LonLat2Screen(loc_start,
		StartP);
  LonLat2Screen(loc_end,
		EndP);

  if (d_distance>distanceTotal) {
    canvas.line(StartP, EndP);
  } else {

    for (int i=0; i<= 10; i++) {

      GEOPOINT t_loc;

      FindLatitudeLongitude(loc_start,
                            Bearing,
                            min(distance,d_distance),
                            &t_loc);

      DistanceBearing(t_loc, loc_end,
                      &distance,
                      &Bearing);

      LonLat2Screen(t_loc,
                    EndP);

      canvas.line(StartP, EndP);

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
  LonLat2Screen(loc_start, pt[0]);
  LonLat2Screen(loc_end, pt[1]);
  canvas.polyline(pt, 2);

#endif
}


///////////


#define MINRANGE 0.2

bool RectangleIsInside(rectObj r_exterior, rectObj r_interior) {
  if ((r_interior.minx >= r_exterior.minx)&&
      (r_interior.maxx <= r_exterior.maxx)&&
      (r_interior.miny >= r_exterior.miny)&&
      (r_interior.maxy <= r_exterior.maxy))
    return true;
  else
    return false;
}

bool MapWindowProjection::SmartBounds(const bool force) {
  rectObj bounds_screen = CalculateScreenBounds(1.0);
  bool recompute = false;

  // only recalculate which shapes when bounds change significantly
  // need to have some trigger for this..

  // trigger if the border goes outside the stored area
  if (!RectangleIsInside(smart_bounds_active, bounds_screen)) {
    recompute = true;
  }

  // also trigger if the scale has changed heaps
  double range_real = max((bounds_screen.maxx-bounds_screen.minx),
			  (bounds_screen.maxy-bounds_screen.miny));
  double range = max(MINRANGE,range_real);

  double scale = range/smart_range_active;
  if (max(scale, 1.0/scale)>4) {
    recompute = true;
  }

  if (recompute || force) {

    // make bounds bigger than screen
    if (range_real<MINRANGE) {
      scale = BORDERFACTOR*MINRANGE/range_real;
    } else {
      scale = BORDERFACTOR;
    }
    smart_bounds_active = CalculateScreenBounds(scale);

    smart_range_active = max((smart_bounds_active.maxx-smart_bounds_active.minx),
			     (smart_bounds_active.maxy-smart_bounds_active.miny));

    // now update visibility of objects in the map window
    return true;
  } else {
    return false;
  }
}



