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
#include "XCSoar.h"
#include "StdAfx.h"
#include "options.h"
#include "Utils.h"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Units.h"
#include "Logger.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"
#include "Dialogs.h"
#include "externs.h"
#include "InputEvents.h"

// #include <assert.h>
#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "Task.h"

//#include "Terrain.h"

#include "GaugeVarioAltA.h"
#include "GaugeCDI.h"
#include "InfoBoxLayout.h"
#include "Screen/Util.hpp"
#include "GlideSolvers.hpp"

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

#ifndef NDEBUG
#if (WINDOWSPC<1)
#define DRAWLOAD
extern HFONT  MapWindowFont;
extern int timestats_av;
extern int misc_tick_count;
#endif
#endif

extern HFONT  MapWindowBoldFont;

extern HFONT  TitleWindowFont;


void MapWindow::DrawCDI() {
  bool dodrawcdi = DerivedDrawInfo.Circling
    ? EnableCDICircling
    : EnableCDICruise;

  if (dodrawcdi) {
    GaugeCDI::Show();
    GaugeCDI::Update(DrawInfo.TrackBearing, DerivedDrawInfo.WaypointBearing);
  } else {
    GaugeCDI::Hide();
  }
}


void MapWindow::ScanVisibility(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)
  const rectObj bounds = *bounds_active;

  // far visibility for snail trail

  ScanVisibilityTrail(bounds_active);

  // far visibility for waypoints

  if (WayPointList) {
    WAYPOINT *wv = WayPointList;
    const WAYPOINT *we = WayPointList+NumberOfWayPoints;
    while (wv<we) {
      // TODO code: optimise waypoint visibility
      wv->FarVisible = ((wv->Longitude> bounds.minx) &&
			(wv->Longitude< bounds.maxx) &&
			(wv->Latitude> bounds.miny) &&
			(wv->Latitude< bounds.maxy));
      wv++;
    }
  }

  // far visibility for airspace

  if (AirspaceCircle) {
    for (AIRSPACE_CIRCLE* circ = AirspaceCircle;
         circ < AirspaceCircle+NumberOfAirspaceCircles; circ++) {
      circ->FarVisible =
        (msRectOverlap(&circ->bounds, bounds_active) == MS_TRUE) ||
        (msRectContained(bounds_active, &circ->bounds) == MS_TRUE) ||
        (msRectContained(&circ->bounds, bounds_active) == MS_TRUE);
    }
  }

  if (AirspaceArea) {
    for(AIRSPACE_AREA *area = AirspaceArea;
        area < AirspaceArea+NumberOfAirspaceAreas; area++) {
      area->FarVisible =
        (msRectOverlap(&area->bounds, bounds_active) == MS_TRUE) ||
        (msRectContained(bounds_active, &area->bounds) == MS_TRUE) ||
        (msRectContained(&area->bounds, bounds_active) == MS_TRUE);
    }
  }

}

#include "SnailTrail.hpp"
// only needed once here!
// TODO: chop this into sections

void MapWindow::CalculateScreenPositions(POINT Orig, RECT rc,
                                         POINT *Orig_Aircraft)
{
  unsigned int i;

  Orig_Screen = Orig;

  if (!EnablePan) {

    if (GliderCenter
        && DerivedDrawInfo.Circling
        && (EnableThermalLocator==2)) {

      if (DerivedDrawInfo.ThermalEstimate_R>0) {
        PanLongitude = DerivedDrawInfo.ThermalEstimate_Longitude;
        PanLatitude = DerivedDrawInfo.ThermalEstimate_Latitude;
        // TODO enhancement: only pan if distance of center to
        // aircraft is smaller than one third screen width

        POINT screen;
        LatLon2Screen(PanLongitude,
                      PanLatitude,
                      screen);

        LatLon2Screen(DrawInfo.Longitude,
                      DrawInfo.Latitude,
                      *Orig_Aircraft);

        if ((fabs((double)Orig_Aircraft->x-screen.x)<(rc.right-rc.left)/3)
            && (fabs((double)Orig_Aircraft->y-screen.y)<(rc.bottom-rc.top)/3)) {

        } else {
          // out of bounds, center on aircraft
          PanLongitude = DrawInfo.Longitude;
          PanLatitude = DrawInfo.Latitude;
        }
      } else {
        PanLongitude = DrawInfo.Longitude;
        PanLatitude = DrawInfo.Latitude;
      }
    } else {
      // Pan is off
      PanLongitude = DrawInfo.Longitude;
      PanLatitude = DrawInfo.Latitude;
    }
  }

  LatLon2Screen(DrawInfo.Longitude,
                DrawInfo.Latitude,
                *Orig_Aircraft);

  screenbounds_latlon = CalculateScreenBounds(0.0);

  // get screen coordinates for all task waypoints

  LockTaskData();

  if (WayPointList) {
    int index;
    for (i=0; i<MAXTASKPOINTS; i++) {
      index = Task[i].Index;
      if (index>=0) {

        LatLon2Screen(WayPointList[index].Longitude,
                      WayPointList[index].Latitude,
                      WayPointList[index].Screen);
        WayPointList[index].Visible =
          PointVisible(WayPointList[index].Screen);
      }
    }
    if (EnableMultipleStartPoints) {
      for(i=0;i<MAXSTARTPOINTS-1;i++) {
        index = StartPoints[i].Index;
        if (StartPoints[i].Active && (index>=0)) {

          LatLon2Screen(WayPointList[index].Longitude,
                        WayPointList[index].Latitude,
                        WayPointList[index].Screen);
          WayPointList[index].Visible =
            PointVisible(WayPointList[index].Screen);
        }
      }
    }

    // only calculate screen coordinates for waypoints that are visible

    for(i=0;i<NumberOfWayPoints;i++)
      {
        WayPointList[i].Visible = false;
        if (!WayPointList[i].FarVisible) continue;
        if(PointVisible(WayPointList[i].Longitude, WayPointList[i].Latitude) )
          {
            LatLon2Screen(WayPointList[i].Longitude, WayPointList[i].Latitude,
                          WayPointList[i].Screen);
            WayPointList[i].Visible = PointVisible(WayPointList[i].Screen);
          }
      }
  }

  if(TrailActive)
  {
    iSnailNext = SnailNext;
    // set this so that new data doesn't arrive between calculating
    // this and the screen updates
  }

  if (EnableMultipleStartPoints) {
    for(i=0;i<MAXSTARTPOINTS-1;i++) {
      if (StartPoints[i].Active && ValidWayPoint(StartPoints[i].Index)) {
        LatLon2Screen(StartPoints[i].SectorEndLon,
                      StartPoints[i].SectorEndLat, StartPoints[i].End);
        LatLon2Screen(StartPoints[i].SectorStartLon,
                      StartPoints[i].SectorStartLat, StartPoints[i].Start);
      }
    }
  }

  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    bool this_valid = ValidTaskPoint(i);
    bool next_valid = ValidTaskPoint(i+1);
    if (AATEnabled && this_valid) {
      LatLon2Screen(Task[i].AATTargetLon, Task[i].AATTargetLat,
                    Task[i].Target);
    }

    if(this_valid && !next_valid)
    {
      // finish
      LatLon2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, Task[i].End);
      LatLon2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, Task[i].Start);
    }
    if(this_valid && next_valid)
    {
      LatLon2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, Task[i].End);
      LatLon2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, Task[i].Start);

      if((AATEnabled) && (Task[i].AATType == SECTOR))
      {
        LatLon2Screen(Task[i].AATStartLon, Task[i].AATStartLat, Task[i].AATStart);
        LatLon2Screen(Task[i].AATFinishLon, Task[i].AATFinishLat, Task[i].AATFinish);
      }
      if (AATEnabled && (((int)i==ActiveWayPoint) ||
			 (TargetPan && ((int)i==TargetPanIndex)))) {

	for (int j=0; j<MAXISOLINES; j++) {
	  if (TaskStats[i].IsoLine_valid[j]) {
	    LatLon2Screen(TaskStats[i].IsoLine_Longitude[j],
			  TaskStats[i].IsoLine_Latitude[j],
			  TaskStats[i].IsoLine_Screen[j]);
	  }
	}
      }
    }
  }

  UnlockTaskData();

}


// JMW this is slow way to do things...

static bool CheckLandableReachableTerrain(NMEA_INFO *Basic,
                                          DERIVED_INFO *Calculated,
                                          double LegToGo,
                                          double LegBearing) {
  double lat, lon;
  bool out_of_range;
  double distance_soarable =
    FinalGlideThroughTerrain(LegBearing,
                             Basic, Calculated,
                             &lat,
                             &lon,
                             LegToGo, &out_of_range, NULL);

  if ((out_of_range)||(distance_soarable> LegToGo)) {
    return true;
  } else {
    return false;
  }
}


void MapWindow::CalculateWaypointReachable(void)
{
  unsigned int i;
  double WaypointDistance, WaypointBearing,AltitudeRequired,AltitudeDifference;

  LandableReachable = false;

  if (!WayPointList) return;

  LockTaskData();

  for(i=0;i<NumberOfWayPoints;i++)
  {
    if ((WayPointList[i].Visible &&
	 (
	  ((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
	  ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT)
	  ))
	|| WaypointInTask(i) ) {

      DistanceBearing(DrawInfo.Latitude,
		      DrawInfo.Longitude,
		      WayPointList[i].Latitude,
		      WayPointList[i].Longitude,
		      &WaypointDistance,
		      &WaypointBearing);

      AltitudeRequired =
	GlidePolar::MacCreadyAltitude
	(GlidePolar::SafetyMacCready,
	 WaypointDistance,
	 WaypointBearing,
	 DerivedDrawInfo.WindSpeed,
	 DerivedDrawInfo.WindBearing,
	 0,0,true,0);
      AltitudeRequired = AltitudeRequired + SAFETYALTITUDEARRIVAL
	+ WayPointList[i].Altitude ;
      AltitudeDifference = DerivedDrawInfo.NavAltitude - AltitudeRequired;
      WayPointList[i].AltArivalAGL = AltitudeDifference;

      if(AltitudeDifference >=0){
	WayPointList[i].Reachable = TRUE;
	if (!LandableReachable || ((int)i==ActiveWayPoint)) {
	  if (CheckLandableReachableTerrain(&DrawInfo,
					    &DerivedDrawInfo,
					    WaypointDistance,
					    WaypointBearing)) {
	    LandableReachable = true;
	  } else if ((int)i==ActiveWayPoint) {
	    WayPointList[i].Reachable = FALSE;
	  }
	}
      } else {
	WayPointList[i].Reachable = FALSE;
      }
    }
  }

  if (!LandableReachable) {
    // widen search to far visible waypoints
    // (only do this if can't see one at present)

    for(i=0;i<NumberOfWayPoints;i++)
      {
        if(!WayPointList[i].Visible && WayPointList[i].FarVisible)
          // visible but only at a distance (limit this to 100km radius)
          {
            if(  ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
                 || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
              {
                DistanceBearing(DrawInfo.Latitude,
                                DrawInfo.Longitude,
                                WayPointList[i].Latitude,
                                WayPointList[i].Longitude,
                                &WaypointDistance,
                                &WaypointBearing);

                if (WaypointDistance<100000.0) {
                  AltitudeRequired =
                    GlidePolar::MacCreadyAltitude
                    (GlidePolar::SafetyMacCready,
                     WaypointDistance,
                     WaypointBearing,
                     DerivedDrawInfo.WindSpeed,
                     DerivedDrawInfo.WindBearing,
                     0,0,true,0);

                  AltitudeRequired = AltitudeRequired + SAFETYALTITUDEARRIVAL
                    + WayPointList[i].Altitude ;
                  AltitudeDifference = DerivedDrawInfo.NavAltitude - AltitudeRequired;
                  WayPointList[i].AltArivalAGL = AltitudeDifference;

                  if(AltitudeDifference >=0){
                    WayPointList[i].Reachable = TRUE;
                    if (!LandableReachable) {
                      if (CheckLandableReachableTerrain(&DrawInfo,
                                                        &DerivedDrawInfo,
                                                        WaypointDistance,
                                                        WaypointBearing)) {
                        LandableReachable = true;
                      }
                    }
                  } else {
                    WayPointList[i].Reachable = FALSE;
                  }
                }
              }
          }
      }
  }

  UnlockTaskData();
}

//////////////////////// TARGET STUFF /////////

// JMW to be used for target preview
bool MapWindow::SetTargetPan(bool do_pan, int target_point) {
  static double old_latitude;
  static double old_longitude;
  static bool old_pan=false;
  static bool old_fullscreen=false;

  if (!TargetPan || (TargetPanIndex != target_point)) {
    TargetDrag_State = 0;
  }

  TargetPanIndex = target_point;

  if (do_pan && !TargetPan) {
    old_latitude = PanLatitude;
    old_longitude = PanLongitude;
    old_pan = EnablePan;
    EnablePan = true;
    TargetPan = do_pan;
    old_fullscreen = RequestFullScreen;
    if (RequestFullScreen) {
      RequestFullScreen = false;
    }
    SwitchZoomClimb();
  }
  if (do_pan) {
    LockTaskData();
    if (ValidTaskPoint(target_point)) {
      PanLongitude = WayPointList[Task[target_point].Index].Longitude;
      PanLatitude = WayPointList[Task[target_point].Index].Latitude;
      if (target_point==0) {
        TargetZoomDistance = max(2e3, StartRadius*2);
      } else if (!ValidTaskPoint(target_point+1)) {
        TargetZoomDistance = max(2e3, FinishRadius*2);
      } else if (AATEnabled) {
        if (Task[target_point].AATType == SECTOR) {
          TargetZoomDistance = max(2e3, Task[target_point].AATSectorRadius*2);
        } else {
          TargetZoomDistance = max(2e3, Task[target_point].AATCircleRadius*2);
        }
      } else {
        TargetZoomDistance = max(2e3, SectorRadius*2);
      }
    }
    UnlockTaskData();
  } else if (TargetPan) {
    PanLongitude = old_longitude;
    PanLatitude = old_latitude;
    EnablePan = old_pan;
    TargetPan = do_pan;
    if (old_fullscreen) {
      RequestFullScreen = true;
    }
    SwitchZoomClimb();
  }
  TargetPan = do_pan;
  return old_pan;
};


bool MapWindow::TargetDragged(double *longitude, double *latitude) {
  bool retval = false;
  LockTaskData();
  if (TargetDrag_State >0) { // always return true if we're dragging or just stopped dragging, so screen is updated
    *longitude = TargetDrag_Longitude;
    *latitude = TargetDrag_Latitude;
    if (TargetDrag_State == 2) {
        TargetDrag_State = 0; // mouse up/ stop dragging
    }
    retval = true;
  }
  UnlockTaskData();
  return retval;
}

