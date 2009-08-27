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
#include "Protection.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Task.h"
#include "WayPoint.hpp"
#include "InfoBoxLayout.h"
#include "SettingsUser.hpp"
#include "Math/Earth.hpp"
#include "Screen/Graphics.hpp"
#include "McReady.h"
#include "GlideSolvers.hpp"
#include "Compatibility/gdi.h"
#include <assert.h>


bool MapWindow::WaypointInTask(int ind) {
  if (!WayPointList) return false;
  return WayPointList[ind].InTask;
}

//FIX
void MapWaypointLabelAdd(TCHAR *Name, int X, int Y,
			 TextInBoxMode_t Mode, int AltArivalAGL,
			 bool inTask, bool isLandable,
			 bool isAirport, bool isExcluded,
			 RECT rc);
void MapWaypointLabelClear();


void MapWindow::DrawWaypoints(HDC hdc, const RECT rc)
{
  unsigned int i;
  TCHAR Buffer[32];
  TCHAR Buffer2[32];
  TCHAR sAltUnit[4];
  TextInBoxMode_t TextDisplayMode;

  // if pan mode, show full names
  int pDisplayTextType = DisplayTextType;
  if (EnablePan) {
    pDisplayTextType = DISPLAYNAME;
  }

  if (!WayPointList) return;

  _tcscpy(sAltUnit, Units::GetAltitudeName());

  MapWaypointLabelClear();

  for(i=0;i<NumberOfWayPoints;i++)
    {
      if(WayPointList[i].Visible )
	{

#ifdef HAVEEXCEPTIONS
	  __try{
#endif

	    bool irange = false;
	    bool intask = false;
	    bool islandable = false;
	    bool dowrite;

	    intask = WaypointInTask(i);
	    dowrite = intask;

	    TextDisplayMode.AsInt = 0;

	    irange = WaypointInRange(i);

	    if(MapScale > 20) {
	      SelectObject(hDCTemp,MapGfx.hSmall);
	    } else if( ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
		       || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) ) {
	      islandable = true; // so we can always draw them
	      if(WayPointList[i].Reachable){

		TextDisplayMode.AsFlag.Reachable = 1;

		if ((DeclutterLabels<2)||intask) {

		  if (intask || (DeclutterLabels<1)) {
		    TextDisplayMode.AsFlag.Border = 1;
		  }
		  // show all reachable landing fields unless we want a decluttered
		  // screen.
		  dowrite = true;
		}

		if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
		  SelectObject(hDCTemp,MapGfx.hBmpAirportReachable);
		else
		  SelectObject(hDCTemp,MapGfx.hBmpFieldReachable);
	      } else {
		if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
		  SelectObject(hDCTemp,MapGfx.hBmpAirportUnReachable);
		else
		  SelectObject(hDCTemp,MapGfx.hBmpFieldUnReachable);
	      }
	    } else {
	      if(MapScale > 4) {
		SelectObject(hDCTemp,MapGfx.hSmall);
	      } else {
		SelectObject(hDCTemp,MapGfx.hTurnPoint);
	      }
	    }

	    if (intask) { // VNT
	      TextDisplayMode.AsFlag.WhiteBold = 1;
	    }

	    if(irange || intask || islandable || dowrite) {

	      DrawBitmapX(hdc,
			  WayPointList[i].Screen.x-IBLSCALE(10),
			  WayPointList[i].Screen.y-IBLSCALE(10),
			  20,20,
			  hDCTemp,0,0,SRCPAINT);

	      DrawBitmapX(hdc,
			  WayPointList[i].Screen.x-IBLSCALE(10),
			  WayPointList[i].Screen.y-IBLSCALE(10),
			  20,20,
			  hDCTemp,20,0,SRCAND);
	    }

	    if(intask || irange || dowrite) {
	      bool draw_alt = TextDisplayMode.AsFlag.Reachable
		&& ((DeclutterLabels<1) || intask);

	      switch(pDisplayTextType) {
	      case DISPLAYNAMEIFINTASK:
		dowrite = intask;
		if (intask) {
		  if (draw_alt)
                    _stprintf(Buffer, TEXT("%s:%d%s"),
                              WayPointList[i].Name,
                              (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                              sAltUnit);
		  else
                    _stprintf(Buffer, TEXT("%s"),WayPointList[i].Name);
		}
		break;
	      case DISPLAYNAME:
		dowrite = (DeclutterLabels<2) || intask;
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%s:%d%s"),
                            WayPointList[i].Name,
                            (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%s"),WayPointList[i].Name);

		break;
	      case DISPLAYNUMBER:
		dowrite = (DeclutterLabels<2) || intask;
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%d:%d%s"),
                            WayPointList[i].Number,
                            (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%d"),WayPointList[i].Number);

		break;
	      case DISPLAYFIRSTFIVE:
		dowrite = (DeclutterLabels<2) || intask;
		_tcsncpy(Buffer2, WayPointList[i].Name, 5);
		Buffer2[5] = '\0';
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%s:%d%s"),
                            Buffer2,
                            (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%s"),Buffer2);

		break;
	      case DISPLAYFIRSTTHREE:
		dowrite = (DeclutterLabels<2) || intask;
		_tcsncpy(Buffer2, WayPointList[i].Name, 3);
		Buffer2[3] = '\0';
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%s:%d%s"),
                            Buffer2,
                            (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%s"),Buffer2);

		break;
	      case DISPLAYNONE:
		dowrite = (DeclutterLabels<2) || intask;
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%d%s"),
                            (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
		  Buffer[0]= '\0';
	      default:
#if (WINDOWSPC<1)
		assert(0);
#endif
		break;
	      }

	      if (dowrite) {
		MapWaypointLabelAdd(
				    Buffer,
				    WayPointList[i].Screen.x+5,
				    WayPointList[i].Screen.y,
				    TextDisplayMode,
				    (int)(WayPointList[i].AltArivalAGL*ALTITUDEMODIFY),
				    intask,false,false,false,
				    MapRect);
	      }

	    }

#ifdef HAVEEXCEPTIONS
	  }__finally
#endif
	     { ; }
	}
    }

  MapWaypointLabelSortAndRender(hdc);
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

  mutexTaskData.Lock();

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

  mutexTaskData.Unlock();
}



void MapWindow::ScanVisibilityWaypoints(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)
  const rectObj bounds = *bounds_active;

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
}


void MapWindow::CalculateScreenPositionsWaypoints() {
  unsigned int i;
  mutexTaskData.Lock();

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
  mutexTaskData.Unlock();
}
