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
#if !defined(XCSOAR_MAPWINDOW_PROJECTION_H)
#define XCSOAR_MAPWINDOW_PROJECTION_H

#include "XCSoar.h"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Screen/Canvas.hpp"
#include "Math/FastMath.h"

class MapWindowData {
 public:
  NMEA_INFO     DrawInfo;
  DERIVED_INFO  DerivedDrawInfo;
};

class MapWindowProjection: public MapWindowData {
 public:
  MapWindowProjection();

  // used by terrain renderer, topology and airspace

  void    Screen2LatLon(const int &x, const int &y,
			       double &X, double &Y);

  void    LatLon2Screen(const double &lon, const double &lat,
			       POINT &sc);
  void    LatLon2Screen(pointObj *ptin, POINT *ptout,
			       const int n,
			       const int skip);

  POINT   GetOrigScreen(void) { return Orig_Screen; }
  double  GetPanLatitude() { return PanLatitude; }
  double  GetPanLongitude() { return PanLongitude; }
  double  GetDisplayAngle() { return DisplayAngle; }

  rectObj CalculateScreenBounds(double scale);

  double  GetScreenDistanceMeters(void);

  double GetScreenScaleToLatLon() {
    return InvDrawScale;
  }
  double GetMapScaleUser() { // Topology
    return MapScale;
  }
  double GetMapScaleKM() {
    return MapScale*0.001/DISTANCEMODIFY;
  }

  bool isPan() {
    return EnablePan;
  }
  bool isTargetPan(void) {
    return TargetPan;
  }
  RECT GetMapRectBig() {
    return MapRectBig;
  }
  RECT GetMapRect() {
    return MapRect;
  }

  // used by waypoint nearest routine
  bool    WaypointInScaleFilter(int i);

  // drawing functions
  void DrawGreatCircle(Canvas &canvas,
		       double lon_start, double lat_start,
		       double lon_end, double lat_end,
		       const RECT rc);

 protected:
  // helpers
  bool PointVisible(const POINT &P);
  bool LonLatVisible(const double &lon, const double &lat);
  bool PointInRect(const double &x, const double &y,
		   const rectObj &bounds);
  rectObj   screenbounds_latlon;
  RECT   MapRectSmall;
  RECT   MapRectBig;
  RECT   MapRect;
  double PanLatitude;
  double PanLongitude;
  POINT  Orig_Screen;

  double DisplayAngle;
  double DisplayAircraftAngle;

  void   CalculateOrigin(const RECT rc, POINT *Orig);

  bool   EnablePan;
  bool   TargetPan;
  int    TargetPanIndex;
  double TargetZoomDistance;

  // scale stuff
  void      UpdateMapScale();

  double    StepMapScale(int Step);
  void      InitialiseScaleList();

  unsigned DistanceMetersToScreen(const double x) {
    return iround(_scale_meters_to_screen*x);
  }

  // 4 = x*30/1000
  double DistancePixelsToMeters(const double x) {
    return x*MapScale/(DISTANCEMODIFY*GetMapResolutionFactor());
  }
  //
  double RequestDistancePixelsToMeters(const double x) {
    return x*_RequestedMapScale/(DISTANCEMODIFY*GetMapResolutionFactor());
  }
  double DistanceScreenToUser(const unsigned x) {
    return x*MapScale/GetMapResolutionFactor();
  }
  double RequestMapScale(double x) {
    _RequestedMapScale = LimitMapScale(x);
    return _RequestedMapScale;
  }
  double GetRequestedMapScale() {
    return _RequestedMapScale;
  }
  double GetLatLonToScreenScale() {
    return DrawScale;
  }
  bool IsOriginCentered() {
    return _origin_centered;
  }
  bool HaveScaleList() {
    return ScaleListCount>0;
  }
 private:

  double DrawScale;
  double InvDrawScale;

  double _scale_meters_to_screen; // speedup
  double MapScaleOverDistanceModify; // speedup
  double MapScale;
  double _RequestedMapScale;

  void   ModifyMapScale();
  void   CalculateOrientationTargetPan(void);
  void   CalculateOrientationNormal(void);
  bool   _origin_centered;
  double    LimitMapScale(double value);
  double    FindMapScale(double Value);
  int       ScaleCurrent;
  double    ScaleList[SCALELISTSIZE];
  int       ScaleListCount;
  int       GetMapResolutionFactor();
};

#endif
