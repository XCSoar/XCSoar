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


void MapWindow::DrawWaypoints(Canvas &canvas)
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

	    irange = WaypointInScaleFilter(i);

	    Bitmap *wp_bmp = &MapGfx.hSmall;

	    if(GetMapScaleKM() > 20) {
	      wp_bmp = &MapGfx.hSmall;
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
		  wp_bmp = &MapGfx.hBmpAirportReachable;
		else
		  wp_bmp = &MapGfx.hBmpFieldReachable;
	      } else {
		if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
		  wp_bmp = &MapGfx.hBmpAirportUnReachable;
		else
		  wp_bmp = &MapGfx.hBmpFieldUnReachable;
	      }
	    } else {
	      if (GetMapScaleKM()>4) {
		wp_bmp = &MapGfx.hTurnPoint;
	      } else {
		wp_bmp = &MapGfx.hSmall;
	      }
            }

	    if (intask) { // VNT
	      TextDisplayMode.AsFlag.WhiteBold = 1;
	    }

	    if(irange || intask || islandable || dowrite) {
	      draw_masked_bitmap(canvas, *wp_bmp, 
				 WayPointList[i].Screen.x,
				 WayPointList[i].Screen.y,
				 20, 20);
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

  MapWaypointLabelSortAndRender(canvas);
}


// JMW this is slow way to do things...

static bool CheckLandableReachableTerrain(const NMEA_INFO *Basic,
                                          const DERIVED_INFO *Calculated,
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

      DistanceBearing(Basic().Latitude,
		      Basic().Longitude,
		      WayPointList[i].Latitude,
		      WayPointList[i].Longitude,
		      &WaypointDistance,
		      &WaypointBearing);

      AltitudeRequired =
	GlidePolar::MacCreadyAltitude
	(GlidePolar::SafetyMacCready,
	 WaypointDistance,
	 WaypointBearing,
	 Calculated().WindSpeed,
	 Calculated().WindBearing,
	 0,0,true,0);
      AltitudeRequired = AltitudeRequired + SAFETYALTITUDEARRIVAL
	+ WayPointList[i].Altitude ;
      AltitudeDifference = Calculated().NavAltitude - AltitudeRequired;
      WayPointList[i].AltArivalAGL = AltitudeDifference;

      if(AltitudeDifference >=0){
	WayPointList[i].Reachable = TRUE;
	if (!LandableReachable || ((int)i==ActiveWayPoint)) {
	  if (CheckLandableReachableTerrain(&Basic(),
					    &Calculated(),
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
                DistanceBearing(Basic().Latitude,
                                Basic().Longitude,
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
                     Calculated().WindSpeed,
                     Calculated().WindBearing,
                     0,0,true,0);

                  AltitudeRequired = AltitudeRequired + SAFETYALTITUDEARRIVAL
                    + WayPointList[i].Altitude ;
                  AltitudeDifference = Calculated().NavAltitude - AltitudeRequired;
                  WayPointList[i].AltArivalAGL = AltitudeDifference;

                  if(AltitudeDifference >=0){
                    WayPointList[i].Reachable = TRUE;
                    if (!LandableReachable) {
                      if (CheckLandableReachableTerrain(&Basic(),
                                                        &Calculated(),
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
  unsigned int j;
  int i;
  mutexTaskData.Lock();

  if (WayPointList) {
    for (j=0; j<MAXTASKPOINTS; j++) {
      i = Task[j].Index;
      if (i>=0) {
	LonLat2Screen(WayPointList[i].Longitude, 
		      WayPointList[i].Latitude,
		      WayPointList[i].Screen);
	WayPointList[i].Visible = PointVisible(WayPointList[i].Screen);
      }
    }
    if (EnableMultipleStartPoints) {
      for(j=0;j<MAXSTARTPOINTS-1;j++) {
        i = StartPoints[j].Index;
        if (StartPoints[j].Active && (i>=0)) {
	  LonLat2Screen(WayPointList[i].Longitude, 
			WayPointList[i].Latitude,
			WayPointList[i].Screen);
	  WayPointList[i].Visible = PointVisible(WayPointList[i].Screen);
        }
      }
    }
    // only calculate screen coordinates for waypoints that are visible
    for(i=0;i<NumberOfWayPoints;i++) {
      if (!WayPointList[i].FarVisible) {
	WayPointList[i].Visible = false;
	continue;
      } else {
	WayPointList[i].Visible = LonLat2ScreenIfVisible(WayPointList[i].Longitude, 
							 WayPointList[i].Latitude,
							 &WayPointList[i].Screen);
      }
    }
  }
  mutexTaskData.Unlock();
}
