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

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Math/FastMath.h"
#include "Units.hpp"

class Canvas;

class MapWindowProjection {
 public:
  MapWindowProjection();

  // used by terrain renderer, topology and airspace

  void    Screen2LonLat(const int &x, const int &y,
                        double &X, double &Y) const;

  void    LonLat2Screen(const double &lon, const double &lat,
                        POINT &sc) const;
  void    LonLat2Screen(const pointObj* const ptin, POINT *ptout,
			       const int n,
                        const int skip) const;

  POINT   GetOrigScreen(void) const { return Orig_Screen; }
  POINT   GetOrigAircraft(void) const { return Orig_Aircraft; }
  double  GetPanLatitude() const { return PanLatitude; }
  double  GetPanLongitude() const { return PanLongitude; }
  double  GetDisplayAngle() const { return DisplayAngle; }

  rectObj CalculateScreenBounds(double scale) const;

  double  GetScreenDistanceMeters(void) const;

  double GetScreenScaleToLonLat() const {
    return InvDrawScale;
  }
  double GetMapScaleUser() const { // Topology
    return MapScale;
  }
  double GetMapScaleKM() const {
    return MapScale*0.001/DISTANCEMODIFY;
  }

  bool isPan() const {
    return EnablePan;
  }
  bool isTargetPan(void) const {
    return TargetPan;
  }
  RECT GetMapRectBig() const {
    return MapRectBig;
  }
  RECT GetMapRect() const {
    return MapRect;
  }

  // used by waypoint nearest routine
  bool WaypointInScaleFilter(int i) const;

  // drawing functions
  void DrawGreatCircle(Canvas &canvas,
		       double lon_start, double lat_start,
		       double lon_end, double lat_end);

  rectObj* getSmartBounds() {
    return &smart_bounds_active;
  }

  // called on receipt of new data, to trigger projection/scale change functions
  void ExchangeBlackboard(const NMEA_INFO &nmea_info,
			  const DERIVED_INFO &derived_info);
 protected:
  // helpers
  bool PointVisible(const POINT &P) const;
  bool LonLatVisible(const double &lon, const double &lat) const;
  bool PointInRect(const double &x, const double &y,
		   const rectObj &bounds) const;

  bool LonLat2ScreenIfVisible(const double &lon, const double &lat,
			      POINT *sc) const;

  rectObj   screenbounds_latlon;
  RECT   MapRectSmall;
  RECT   MapRectBig;
  RECT   MapRect;
  double PanLatitude;
  double PanLongitude;
  POINT  Orig_Screen, Orig_Aircraft;

  double DisplayAngle;
  double DisplayAircraftAngle;

  bool   EnablePan;
  bool   TargetPan;
  int    TargetPanIndex;
  double TargetZoomDistance;

  // scale/display stuff
  void   CalculateOrigin(const RECT rc,
			 const NMEA_INFO &nmea_info,
			 const DERIVED_INFO &derived_info);

  double    StepMapScale(int Step);
  void      InitialiseScaleList();

  unsigned DistanceMetersToScreen(const double x) {
    return iround(_scale_meters_to_screen*x);
  }

  // 4 = x*30/1000
  double DistancePixelsToMeters(const double x) const {
    return x*MapScale/(DISTANCEMODIFY*GetMapResolutionFactor());
  }
  //
  double RequestDistancePixelsToMeters(const double x) const {
    return x*_RequestedMapScale/(DISTANCEMODIFY*GetMapResolutionFactor());
  }
  double DistanceScreenToUser(const unsigned x) const {
    return x*MapScale/GetMapResolutionFactor();
  }
  double RequestMapScale(double x) {
    _RequestedMapScale = LimitMapScale(x);
    return _RequestedMapScale;
  }
  double GetRequestedMapScale() const {
    return _RequestedMapScale;
  }
  double GetLonLatToScreenScale() const {
    return DrawScale;
  }
  bool IsOriginCentered() const {
    return _origin_centered;
  }
  bool HaveScaleList() const {
    return ScaleListCount>0;
  }

  bool SmartBounds(const bool force);

 private:

  double DrawScale;
  double InvDrawScale;

  double _scale_meters_to_screen; // speedup
  double MapScaleOverDistanceModify; // speedup
  double MapScale;
  double _RequestedMapScale;

  void   ModifyMapScale();

  void   UpdateMapScale(const NMEA_INFO &nmea_info,
			const DERIVED_INFO &derived_info);
  void   CalculateOrientationTargetPan(const NMEA_INFO &nmea_info,
				       const DERIVED_INFO &derived_info);
  void   CalculateOrientationNormal(const NMEA_INFO &nmea_info,
				    const DERIVED_INFO &derived_info);

  bool   _origin_centered;
  double    LimitMapScale(double value);
  double    FindMapScale(double Value);
  int       ScaleCurrent;
  double    ScaleList[SCALELISTSIZE];
  int       ScaleListCount;
  int GetMapResolutionFactor() const;

  rectObj smart_bounds_active;
  double smart_range_active;
};

#endif
