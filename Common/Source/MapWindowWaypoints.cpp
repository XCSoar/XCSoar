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
#include "externs.h"
#include "Task.h"
#include "WayPoint.hpp"
#include "InfoBoxLayout.h"
//#include "Screen/Util.hpp"
//#include "Math/Screen.hpp"
#include "Math/Earth.hpp"


bool MapWindow::WaypointInTask(int ind) {
  if (!WayPointList) return false;
  return WayPointList[ind].InTask;
}

//FIX
//static void MapWaypointLabelAdd(TCHAR *Name, int X, int Y, TextInBoxMode_t Mode, int AltArivalAGL, bool inTask=false, bool isLandable=false, bool isAirport=false, bool isExcluded=false);
void MapWaypointLabelAdd(TCHAR *Name, int X, int Y, TextInBoxMode_t Mode, int AltArivalAGL, bool inTask, bool isLandable, bool isAirport, bool isExcluded);
void MapWaypointLabelClear();


bool MapWindow::WaypointInRange(int i) {
  return ((WayPointList[i].Zoom >= MapScale*10)
          || (WayPointList[i].Zoom == 0))
    && (MapScale <= 10);
}

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
	      SelectObject(hDCTemp,hSmall);
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
		  SelectObject(hDCTemp,hBmpAirportReachable);
		else
		  SelectObject(hDCTemp,hBmpFieldReachable);
	      } else {
		if ((WayPointList[i].Flags & AIRPORT) == AIRPORT)
		  SelectObject(hDCTemp,hBmpAirportUnReachable);
		else
		  SelectObject(hDCTemp,hBmpFieldUnReachable);
	      }
	    } else {
	      if(MapScale > 4) {
		SelectObject(hDCTemp,hSmall);
	      } else {
		SelectObject(hDCTemp,hTurnPoint);
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
				    intask,false,false,false);
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
