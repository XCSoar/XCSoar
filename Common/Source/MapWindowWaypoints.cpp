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
#include "Compatibility/gdi.h"
#include <assert.h>



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
  int pDisplayTextType = SettingsMap().DisplayTextType;
  if (SettingsMap().EnablePan) {
    pDisplayTextType = DISPLAYNAME;
  }

  if (!WayPointList) return;

  _tcscpy(sAltUnit, Units::GetAltitudeName());

  MapWaypointLabelClear();

  for(i=0;i<NumberOfWayPoints;i++)
    {
      const WAYPOINT &way_point = WayPointList[i];
      const WPCALC &wpcalc = WayPointCalc[i];

      if (wpcalc.Visible) {

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
            } else if( ((way_point.Flags & AIRPORT) == AIRPORT)
                       || ((way_point.Flags & LANDPOINT) == LANDPOINT) ) {
	      islandable = true; // so we can always draw them
              if (wpcalc.Reachable) {

		TextDisplayMode.AsFlag.Reachable = 1;

		if ((SettingsMap().DeclutterLabels<2)||intask) {

		  if (intask || (SettingsMap().DeclutterLabels<1)) {
		    TextDisplayMode.AsFlag.Border = 1;
		  }
		  // show all reachable landing fields unless we want a decluttered
		  // screen.
		  dowrite = true;
		}

                if ((way_point.Flags & AIRPORT) == AIRPORT)
		  wp_bmp = &MapGfx.hBmpAirportReachable;
		else
		  wp_bmp = &MapGfx.hBmpFieldReachable;
	      } else {
                if ((way_point.Flags & AIRPORT) == AIRPORT)
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
                                 wpcalc.Screen.x, wpcalc.Screen.y,
				 20, 20);
            }

	    if(intask || irange || dowrite) {
	      bool draw_alt = TextDisplayMode.AsFlag.Reachable
		&& ((SettingsMap().DeclutterLabels<1) || intask);

	      switch(pDisplayTextType) {
	      case DISPLAYNAMEIFINTASK:
		dowrite = intask;
		if (intask) {
		  if (draw_alt)
                    _stprintf(Buffer, TEXT("%s:%d%s"),
                              way_point.Name,
                              (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
                              sAltUnit);
		  else
                    _stprintf(Buffer, TEXT("%s"),way_point.Name);
		}
		break;
	      case DISPLAYNAME:
		dowrite = (SettingsMap().DeclutterLabels<2) || intask;
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%s:%d%s"),
                            way_point.Name,
                            (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%s"),way_point.Name);

		break;
	      case DISPLAYNUMBER:
		dowrite = (SettingsMap().DeclutterLabels<2) || intask;
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%d:%d%s"),
                            way_point.Number,
                            (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%d"),way_point.Number);

		break;
	      case DISPLAYFIRSTFIVE:
		dowrite = (SettingsMap().DeclutterLabels<2) || intask;
		_tcsncpy(Buffer2, way_point.Name, 5);
		Buffer2[5] = '\0';
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%s:%d%s"),
                            Buffer2,
                            (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%s"),Buffer2);

		break;
	      case DISPLAYFIRSTTHREE:
		dowrite = (SettingsMap().DeclutterLabels<2) || intask;
		_tcsncpy(Buffer2, way_point.Name, 3);
		Buffer2[3] = '\0';
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%s:%d%s"),
                            Buffer2,
                            (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
                  _stprintf(Buffer, TEXT("%s"),Buffer2);

		break;
	      case DISPLAYNONE:
		dowrite = (SettingsMap().DeclutterLabels<2) || intask;
		if (draw_alt)
                  _stprintf(Buffer, TEXT("%d%s"),
                            (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
                            sAltUnit);
		else
		  Buffer[0]= '\0';
	      default:
#ifndef WINDOWSPC
		assert(0);
#endif
		break;
	      }

	      if (dowrite) {
		MapWaypointLabelAdd(
				    Buffer,
                                    wpcalc.Screen.x + 5, wpcalc.Screen.y,
				    TextDisplayMode,
                                    (int)(wpcalc.AltArrivalAGL*ALTITUDEMODIFY),
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


void MapWindow::ScanVisibilityWaypoints(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)
  const rectObj bounds = *bounds_active;

  if (WayPointList) {
    WAYPOINT *wv = WayPointList;
    WPCALC *wc = WayPointCalc;
    const WAYPOINT *we = WayPointList+NumberOfWayPoints;
    while (wv<we) {
      // TODO code: optimise waypoint visibility
      wc->FarVisible = ((wv->Location.Longitude> bounds.minx) &&
			(wv->Location.Longitude< bounds.maxx) &&
			(wv->Location.Latitude> bounds.miny) &&
			(wv->Location.Latitude< bounds.maxy));
      wv++; wc++;
    }
  }
}


void MapWindow::CalculateScreenPositionsWaypoints() {
  unsigned int j;
  mutexTaskData.Lock();

  if (WayPointList) {
    for (j=0; j<MAXTASKPOINTS; j++) {
      int i = task_points[j].Index;
      if (i>=0) {
        WPCALC &wpcalc = WayPointCalc[i];
	LonLat2Screen(WayPointList[i].Location, 
                      wpcalc.Screen);
        wpcalc.Visible = PointVisible(wpcalc.Screen);
      }
    }
    if (EnableMultipleStartPoints) {
      for(j=0;j<MAXSTARTPOINTS-1;j++) {
        int i = task_start_points[j].Index;
        if (task_start_stats[j].Active && (i>=0)) {
          WPCALC &wpcalc = WayPointCalc[i];
	  LonLat2Screen(WayPointList[i].Location, 
                        wpcalc.Screen);
          wpcalc.Visible = PointVisible(wpcalc.Screen);
        }
      }
    }
    // only calculate screen coordinates for waypoints that are visible
    for (unsigned i = 0; i < NumberOfWayPoints; i++) {
      WPCALC &wpcalc = WayPointCalc[i];
      if (!wpcalc.FarVisible) {
        wpcalc.Visible = false;
	continue;
      } else {
        wpcalc.Visible = LonLat2ScreenIfVisible(WayPointList[i].Location,
                                                &wpcalc.Screen);
      }
    }
  }
  mutexTaskData.Unlock();
}
