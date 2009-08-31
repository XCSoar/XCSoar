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

class MapWindowData {
 public:
  static NMEA_INFO     DrawInfo;
  static DERIVED_INFO  DerivedDrawInfo;
};

class MapWindowProjection: public MapWindowData {
 public:
  // used by terrain renderer, topology and airspace
  static void    OrigScreen2LatLon(const int &x, const int &y,
                                double &X, double &Y);
  static void    Screen2LatLon(const int &x, const int &y,
			       double &X, double &Y);

  static void    LatLon2Screen(const double &lon, const double &lat,
			       POINT &sc);
  static void    LatLon2Screen(pointObj *ptin, POINT *ptout,
			       const int n,
			       const int skip);

  static POINT   GetOrigScreen(void) { return Orig_Screen; }
  static double  GetPanLatitude() { return PanLatitude; }
  static double  GetPanLongitude() { return PanLongitude; }
  static double  GetDisplayAngle() { return DisplayAngle; }
  static double  GetInvDrawScale() { return InvDrawScale; }
  static rectObj CalculateScreenBounds(double scale);
  static double  GetApproxScreenRange(void);

  static double GetMapScale() { // Topology
    return MapScale;
  }
  static int     GetMapResolutionFactor();

  static bool isPan() {
    return EnablePan;
  }
  static bool isTargetPan(void) {
    return TargetPan;
  }
  static RECT GetMapRectBig() {
    return MapRectBig;
  }
  static RECT GetMapRect() {
    return MapRect;
  }

  // used by waypoint nearest routine
  static bool    WaypointInRange(int i);

  // drawing functions
  static void DrawGreatCircle(Canvas &canvas,
			      double lon_start, double lat_start,
			      double lon_end, double lat_end,
			      const RECT rc);

 protected:
  // helpers
  static bool PointVisible(const POINT &P);
  static bool PointVisible(const double &lon, const double &lat);
  static bool PointInRect(const double &lon, const double &lat,
			  const rectObj &bounds);
  static rectObj   screenbounds_latlon;
  static RECT   MapRectSmall;
  static RECT   MapRectBig;
  static RECT   MapRect;
  static double PanLatitude;
  static double PanLongitude;
  static POINT  Orig_Screen;
  static double DrawScale;
  static double InvDrawScale;
  static double DisplayAngle;
  static bool   GliderCenter;
  static double MapScale;
  static double DisplayAircraftAngle;
  static double MapScaleOverDistanceModify; // speedup
  static double ResMapScaleOverDistanceModify; // speedup
  static double RequestMapScale;

  static void   CalculateOrigin(const RECT rc, POINT *Orig);

  static bool   EnablePan;
  static bool   TargetPan;
  static int    TargetPanIndex;
  static double TargetZoomDistance;

  // scale stuff
  static void      UpdateMapScale();

  static int       ScaleListCount;
  static int       ScaleCurrent;
  static double    ScaleList[SCALELISTSIZE];
  static double    StepMapScale(int Step);
  static double    FindMapScale(double Value);
  static double    LimitMapScale(double value);
  static void      InitialiseScaleList();

 private:
  static void   ModifyMapScale();
  static void   CalculateOrientationTargetPan(void);
  static void   CalculateOrientationNormal(void);
};

#endif
