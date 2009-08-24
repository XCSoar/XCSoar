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
#include "Task.h"
#include "SettingsTask.hpp"
#include "WayPoint.hpp"
#include "Screen/Util.hpp"
#include "Screen/Graphics.hpp"
#include "InfoBoxLayout.h"
#include "AATDistance.h"
#include "Math/FastMath.h"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "Compatibility/gdi.h"
#include <math.h>

static const COLORREF taskcolor = RGB(0,120,0); // duplicated from MapWindow.cpp!


void MapWindow::DrawAbortedTask(HDC hdc, const RECT rc, const POINT me)
{
  int i;
  if (!WayPointList) return;

  LockTaskData();  // protect from external task changes
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    for(i=0;i<MAXTASKPOINTS-1;i++)
      {
	int index = Task[i].Index;
	if(ValidWayPoint(index))
	  {
	    DrawDashLine(hdc, 1,
			 WayPointList[index].Screen,
			 me,
			 taskcolor, rc);
	  }
      }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnlockTaskData();
     }
}


void MapWindow::DrawStartSector(HDC hdc, const RECT rc,
                                POINT &Start,
                                POINT &End, int Index) {
  double tmp;

  if(StartLine) {
    _DrawLine(hdc, PS_SOLID, IBLSCALE(5), WayPointList[Index].Screen,
              Start, taskcolor, rc);
    _DrawLine(hdc, PS_SOLID, IBLSCALE(5), WayPointList[Index].Screen,
              End, taskcolor, rc);
    _DrawLine(hdc, PS_SOLID, IBLSCALE(1), WayPointList[Index].Screen,
              Start, RGB(255,0,0), rc);
    _DrawLine(hdc, PS_SOLID, IBLSCALE(1), WayPointList[Index].Screen,
              End, RGB(255,0,0), rc);
  } else {
    tmp = StartRadius*ResMapScaleOverDistanceModify;
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    SelectObject(hdc, MapGfx.hpStartFinishThick);
    Circle(hdc,
           WayPointList[Index].Screen.x,
           WayPointList[Index].Screen.y,(int)tmp, rc, false, false);
    SelectObject(hdc, MapGfx.hpStartFinishThin);
    Circle(hdc,
           WayPointList[Index].Screen.x,
           WayPointList[Index].Screen.y,(int)tmp, rc, false, false);
  }

}


void MapWindow::DrawTask(HDC hdc, RECT rc, const POINT &Orig_Aircraft)
{
  int i;
  double tmp;

  COLORREF whitecolor = RGB(0xff,0xff, 0xff);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  if (!WayPointList) return;

  LockTaskData();  // protect from external task changes
#ifdef HAVEEXCEPTIONS
  __try{
#endif

    if(ValidTaskPoint(0) && ValidTaskPoint(1) && (ActiveWayPoint<2))
      {
	DrawStartSector(hdc,rc, Task[0].Start, Task[0].End, Task[0].Index);
	if (EnableMultipleStartPoints) {
	  for (i=0; i<MAXSTARTPOINTS; i++) {
	    if (StartPoints[i].Active && ValidWayPoint(StartPoints[i].Index)) {
	      DrawStartSector(hdc,rc,
			      StartPoints[i].Start,
			      StartPoints[i].End, StartPoints[i].Index);
	    }
	  }
	}
      }

    for(i=1;i<MAXTASKPOINTS-1;i++) {

      if(ValidTaskPoint(i) && !ValidTaskPoint(i+1)) { // final waypoint
	if (ActiveWayPoint>1) {
	  // only draw finish line when past the first
	  // waypoint.
	  if(FinishLine) {
	    _DrawLine(hdc, PS_SOLID, IBLSCALE(5),
		      WayPointList[Task[i].Index].Screen,
		      Task[i].Start, taskcolor, rc);
	    _DrawLine(hdc, PS_SOLID, IBLSCALE(5),
		      WayPointList[Task[i].Index].Screen,
		      Task[i].End, taskcolor, rc);
	    _DrawLine(hdc, PS_SOLID, IBLSCALE(1),
		      WayPointList[Task[i].Index].Screen,
		      Task[i].Start, RGB(255,0,0), rc);
	    _DrawLine(hdc, PS_SOLID, IBLSCALE(1),
		      WayPointList[Task[i].Index].Screen,
		      Task[i].End, RGB(255,0,0), rc);
	  } else {
	    tmp = FinishRadius*ResMapScaleOverDistanceModify;
	    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	    SelectObject(hdc, MapGfx.hpStartFinishThick);
	    Circle(hdc,
		   WayPointList[Task[i].Index].Screen.x,
		   WayPointList[Task[i].Index].Screen.y,
		   (int)tmp, rc, false, false);
	    SelectObject(hdc, MapGfx.hpStartFinishThin);
	    Circle(hdc,
		   WayPointList[Task[i].Index].Screen.x,
		   WayPointList[Task[i].Index].Screen.y,
		   (int)tmp, rc, false, false);
	  }
	}
      }
      if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) { // normal sector
	if(AATEnabled != TRUE) {
	  DrawDashLine(hdc, 2,
		       WayPointList[Task[i].Index].Screen,
		       Task[i].Start, RGB(127,127,127), rc);
	  DrawDashLine(hdc, 2,
		       WayPointList[Task[i].Index].Screen,
		       Task[i].End, RGB(127,127,127), rc);

	  SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
	  SelectObject(hdc, GetStockObject(BLACK_PEN));
	  if(SectorType== 0) {
	    tmp = SectorRadius*ResMapScaleOverDistanceModify;
	    Circle(hdc,
		   WayPointList[Task[i].Index].Screen.x,
		   WayPointList[Task[i].Index].Screen.y,
		   (int)tmp, rc, false, false);
	  }
	  if(SectorType==1) {
	    tmp = SectorRadius*ResMapScaleOverDistanceModify;
	    Segment(hdc,
		    WayPointList[Task[i].Index].Screen.x,
		    WayPointList[Task[i].Index].Screen.y,(int)tmp, rc,
		    Task[i].AATStartRadial-DisplayAngle,
		    Task[i].AATFinishRadial-DisplayAngle);
	  }
	  if(SectorType== 2) {
	    // JMW added german rules
	    tmp = 500*ResMapScaleOverDistanceModify;
	    Circle(hdc,
		   WayPointList[Task[i].Index].Screen.x,
		   WayPointList[Task[i].Index].Screen.y,
		   (int)tmp, rc, false, false);

	    tmp = 10e3*ResMapScaleOverDistanceModify;

	    Segment(hdc,
		    WayPointList[Task[i].Index].Screen.x,
		    WayPointList[Task[i].Index].Screen.y,(int)tmp, rc,
		    Task[i].AATStartRadial-DisplayAngle,
		    Task[i].AATFinishRadial-DisplayAngle);

	  }
	} else {
	  // JMW added iso lines
	  if ((i==ActiveWayPoint) || (TargetPan && (i==TargetPanIndex))) {
	    // JMW 20080616 flash arc line if very close to target
	    static bool flip = false;

	    if (DerivedDrawInfo.WaypointDistance<AATCloseDistance()*2.0) {
	      flip = !flip;
	    } else {
	      flip = true;
	    }
	    if (flip) {
	      for (int j=0; j<MAXISOLINES-1; j++) {
		if (TaskStats[i].IsoLine_valid[j]
		    && TaskStats[i].IsoLine_valid[j+1]) {
		  _DrawLine(hdc, PS_SOLID, IBLSCALE(2),
			    TaskStats[i].IsoLine_Screen[j],
			    TaskStats[i].IsoLine_Screen[j+1],
			    RGB(0,0,255), rc);
		}
	      }
	    }
	  }
	}
      }
    }

    for(i=0;i<MAXTASKPOINTS-1;i++) {
      if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
	bool is_first = (Task[i].Index < Task[i+1].Index);
	int imin = min(Task[i].Index,Task[i+1].Index);
	int imax = max(Task[i].Index,Task[i+1].Index);
	// JMW AAT!
	double bearing = Task[i].OutBound;
	POINT sct1, sct2;
	if (AATEnabled && !TargetPan) {
	  LatLon2Screen(Task[i].AATTargetLon,
			Task[i].AATTargetLat,
			sct1);
	  LatLon2Screen(Task[i+1].AATTargetLon,
			Task[i+1].AATTargetLat,
			sct2);
	  DistanceBearing(Task[i].AATTargetLat,
			  Task[i].AATTargetLon,
			  Task[i+1].AATTargetLat,
			  Task[i+1].AATTargetLon,
			  NULL, &bearing);

	  // draw nominal track line
	  DrawDashLine(hdc, 1,
		       WayPointList[imin].Screen,
		       WayPointList[imax].Screen,
		       taskcolor, rc);
	} else {
	  sct1 = WayPointList[Task[i].Index].Screen;
	  sct2 = WayPointList[Task[i+1].Index].Screen;
	}

	if (is_first) {
	  DrawDashLine(hdc, 3,
		       sct1,
		       sct2,
		       taskcolor, rc);
	} else {
	  DrawDashLine(hdc, 3,
		       sct2,
		       sct1,
		       taskcolor, rc);
	}

	// draw small arrow along task direction
	POINT p_p;
	POINT Arrow[2] = { {6,6}, {-6,6} };
	ScreenClosestPoint(sct1, sct2,
			   Orig_Aircraft, &p_p, IBLSCALE(25));
	PolygonRotateShift(Arrow, 2, p_p.x, p_p.y,
			   bearing-DisplayAngle);

	_DrawLine(hdc, PS_SOLID, IBLSCALE(2), Arrow[0], p_p, taskcolor, rc);
	_DrawLine(hdc, PS_SOLID, IBLSCALE(2), Arrow[1], p_p, taskcolor, rc);
      }
    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnlockTaskData();
     }

  // restore original color
  SetTextColor(hDCTemp, origcolor);

}


void MapWindow::DrawTaskAAT(HDC hdc, const RECT rc, HDC buffer)
{
  int i;
  double tmp;

  if (!WayPointList) return;
  if (!AATEnabled) return;

  LockTaskData();  // protect from external task changes
#ifdef HAVEEXCEPTIONS
  __try{
#endif

    COLORREF whitecolor = RGB(0xff,0xff, 0xff);
    COLORREF origcolor = SetTextColor(buffer, whitecolor);

    SelectObject(buffer, GetStockObject(WHITE_PEN));
    SelectObject(buffer, GetStockObject(WHITE_BRUSH));
    Rectangle(buffer,rc.left,rc.top,rc.right,rc.bottom);

    for(i=MAXTASKPOINTS-2;i>0;i--)
      {
	if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
	  if(Task[i].AATType == CIRCLE)
	    {
	      tmp = Task[i].AATCircleRadius*ResMapScaleOverDistanceModify;

	      // this color is used as the black bit
              SetTextColor(buffer,
			   MapGfx.Colours[iAirspaceColour[AATASK]]);

	      // this color is the transparent bit
              SetBkColor(buffer,
			 whitecolor);

	      if (i<ActiveWayPoint) {
                SelectObject(buffer, GetStockObject(HOLLOW_BRUSH));
	      } else {
                SelectObject(buffer, MapGfx.hAirspaceBrushes[iAirspaceBrush[AATASK]]);
	      }
              SelectObject(buffer, GetStockObject(BLACK_PEN));

              Circle(buffer,
		     WayPointList[Task[i].Index].Screen.x,
		     WayPointList[Task[i].Index].Screen.y,
		     (int)tmp, rc, true, true);
	    }
	  else
	    {

	      // this color is used as the black bit
              SetTextColor(buffer,
			   MapGfx.Colours[iAirspaceColour[AATASK]]);

	      // this color is the transparent bit
              SetBkColor(buffer,
			 whitecolor);

	      if (i<ActiveWayPoint) {
                SelectObject(buffer, GetStockObject(HOLLOW_BRUSH));
	      } else {
                SelectObject(buffer, MapGfx.hAirspaceBrushes[iAirspaceBrush[AATASK]]);
	      }
              SelectObject(buffer, GetStockObject(BLACK_PEN));

	      tmp = Task[i].AATSectorRadius*ResMapScaleOverDistanceModify;

              Segment(buffer,
		      WayPointList[Task[i].Index].Screen.x,
		      WayPointList[Task[i].Index].Screen.y,(int)tmp, rc,
		      Task[i].AATStartRadial-DisplayAngle,
		      Task[i].AATFinishRadial-DisplayAngle);

              ClipLine(buffer,
                       WayPointList[Task[i].Index].Screen, Task[i].AATStart,
                       rc);
              ClipLine(buffer,
                       WayPointList[Task[i].Index].Screen, Task[i].AATFinish,
                       rc);

	    }

	}
      }

    // restore original color
    SetTextColor(buffer, origcolor);

    //////

#if (WINDOWSPC<1)
    TransparentImage(hdc,
		     rc.left,rc.top,
		     rc.right-rc.left,rc.bottom-rc.top,
                     buffer,
		     rc.left,rc.top,
		     rc.right-rc.left,rc.bottom-rc.top,
		     whitecolor
		     );

#else
    TransparentBlt(hdc,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   buffer,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );
#endif

#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnlockTaskData();
     }
}


void MapWindow::DrawBearing(HDC hdc, const RECT rc, int bBearingValid)
{ /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */

  if (!ValidTaskPoint(ActiveWayPoint)) {
    return;
  }

  LockTaskData();  // protect from external task changes

  double startLat = DrawInfo.Latitude;
  double startLon = DrawInfo.Longitude;
  double targetLat;
  double targetLon;

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)) {
    targetLat = Task[ActiveWayPoint].AATTargetLat;
    targetLon = Task[ActiveWayPoint].AATTargetLon;
  } else {
    targetLat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    targetLon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
  }
  UnlockTaskData();
  if (bBearingValid) {
      DrawGreatCircle(hdc, startLon, startLat,  // RLD skip if bearing invalid
                      targetLon, targetLat, rc);// RLD bc Lat/Lon invalid

    if (TargetPan) {
      // Draw all of task if in target pan mode
      startLat = targetLat;
      startLon = targetLon;

      LockTaskData();

      for (int i=ActiveWayPoint+1; i<MAXTASKPOINTS; i++) {
        if (ValidTaskPoint(i)) {

          if (AATEnabled && ValidTaskPoint(i+1)) {
            targetLat = Task[i].AATTargetLat;
            targetLon = Task[i].AATTargetLon;
          } else {
            targetLat = WayPointList[Task[i].Index].Latitude;
            targetLon = WayPointList[Task[i].Index].Longitude;
          }

          DrawGreatCircle(hdc, startLon, startLat,
                          targetLon, targetLat, rc);

          startLat = targetLat;
          startLon = targetLon;
        }
      }

      UnlockTaskData();

    } // TargetPan
  } // bearing valid

  // JMW draw symbol at target, makes it easier to see
  // RLD always draw all targets ahead so visible in pan mode
  if (AATEnabled) {
    LockTaskData();
    for (int i=ActiveWayPoint; i<MAXTASKPOINTS; i++) {
      // RLD skip invalid targets and targets at start and finish
      if(ValidTaskPoint(i) && ValidTaskPoint(i+1) && i > 0) {
        if (i>= ActiveWayPoint) {
          POINT sct;
          LatLon2Screen(Task[i].AATTargetLon,
                        Task[i].AATTargetLat,
                        sct);
          DrawBitmapIn(hdc, sct, MapGfx.hBmpTarget);
        }
      }
    }
    UnlockTaskData();
  }

}



extern HFONT  TitleWindowFont;

void MapWindow::DrawOffTrackIndicator(HDC hdc, const RECT rc) {
  if ((ActiveWayPoint<=0) || !ValidTaskPoint(ActiveWayPoint)) {
    return;
  }
  if (fabs(DrawInfo.TrackBearing-DerivedDrawInfo.WaypointBearing)<10) {
    // insignificant error
    return;
  }
  if (DerivedDrawInfo.Circling || TaskIsTemporary() || TargetPan) {
    // don't display in various modes
    return;
  }

  double distance_max = min(DerivedDrawInfo.WaypointDistance,
			    GetApproxScreenRange()*0.7);
  if (distance_max < 5000.0) {
    // too short to bother
    return;
  }

  LockTaskData();  // protect from external task changes

  double startLat = DrawInfo.Latitude;
  double startLon = DrawInfo.Longitude;
  double targetLat;
  double targetLon;
  double dLat, dLon;

  if (AATEnabled && ValidTaskPoint(ActiveWayPoint+1)) {
    targetLat = Task[ActiveWayPoint].AATTargetLat;
    targetLon = Task[ActiveWayPoint].AATTargetLon;
  } else {
    targetLat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    targetLon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
  }
  UnlockTaskData();

  HFONT oldFont = (HFONT)SelectObject(hdc, TitleWindowFont);
  SetTextColor(hdc, RGB(0x0,0x0,0x0));

  int ilast = 0;
  for (double d=0.25; d<=1.0; d+= 0.25) {
    double distance0, distance1;

    FindLatitudeLongitude(startLat, startLon,
			  DrawInfo.TrackBearing,
			  distance_max*d,
			  &dLat, &dLon);

    DistanceBearing(startLat, startLon,
		    dLat, dLon,
		    &distance0,
		    NULL);
    DistanceBearing(dLat, dLon,
		    targetLat, targetLon,
		    &distance1,
		    NULL);

    double distance = (distance0+distance1)/DerivedDrawInfo.WaypointDistance;
    int idist = iround((distance-1.0)*100);

    if ((idist != ilast) && (idist>0) && (idist<1000)) {

      TCHAR Buffer[5];
      _stprintf(Buffer, TEXT("%d"), idist);
      short size = _tcslen(Buffer);
      SIZE tsize;
      POINT sc;
      RECT brect;
      LatLon2Screen(dLon, dLat, sc);
      GetTextExtentPoint(hdc, Buffer, size, &tsize);

      brect.left = sc.x-4;
      brect.right = brect.left+tsize.cx+4;
      brect.top = sc.y-4;
      brect.bottom = brect.top+tsize.cy+4;

      if (checkLabelBlock(brect)) {
	ExtTextOut(hdc, sc.x-tsize.cx/2, sc.y-tsize.cy/2,
		   0, NULL, Buffer, size, NULL);
	ilast = idist;
      }
    }

  }

  SelectObject(hdc, oldFont);
}



void MapWindow::DrawProjectedTrack(HDC hdc, const RECT rc, const POINT Orig) {
  if ((ActiveWayPoint<=0) || !ValidTaskPoint(ActiveWayPoint) || !AATEnabled) {
    return;
  }
  if (DerivedDrawInfo.Circling || TaskIsTemporary()) {
    // don't display in various modes
    return;
  }

  // TODO feature: maybe have this work even if no task?
  // TODO feature: draw this also when in target pan mode

  LockTaskData();  // protect from external task changes

  double startLat = DrawInfo.Latitude;
  double startLon = DrawInfo.Longitude;
  double previousLat;
  double previousLon;
  if (AATEnabled) {
    previousLat = Task[max(0,ActiveWayPoint-1)].AATTargetLat;
    previousLon = Task[max(0,ActiveWayPoint-1)].AATTargetLon;
  } else {
    previousLat = WayPointList[Task[max(0,ActiveWayPoint-1)].Index].Latitude;
    previousLon = WayPointList[Task[max(0,ActiveWayPoint-1)].Index].Longitude;
  }
  UnlockTaskData();

  double distance_from_previous, bearing;
  DistanceBearing(previousLat, previousLon,
		  startLat, startLon,
		  &distance_from_previous,
		  &bearing);

  if (distance_from_previous < 100.0) {
    bearing = DrawInfo.TrackBearing;
    // too short to have valid data
  }
  POINT pt[2] = {{0,-75},{0,-400}};
  if (TargetPan) {
    double screen_range = GetApproxScreenRange();
    double flow = 0.4;
    double fhigh = 1.5;
    screen_range = max(screen_range, DerivedDrawInfo.WaypointDistance);

    double p1Lat;
    double p1Lon;
    double p2Lat;
    double p2Lon;
    FindLatitudeLongitude(startLat, startLon,
			  bearing, flow*screen_range,
			  &p1Lat, &p1Lon);
    FindLatitudeLongitude(startLat, startLon,
			  bearing, fhigh*screen_range,
			  &p2Lat, &p2Lon);
    LatLon2Screen(p1Lon, p1Lat, pt[0]);
    LatLon2Screen(p2Lon, p2Lat, pt[1]);
  } else if (fabs(bearing-DerivedDrawInfo.WaypointBearing)<10) {
    // too small an error to bother
    return;
  } else {
    pt[1].y = (long)(-max(MapRectBig.right-MapRectBig.left,
			  MapRectBig.bottom-MapRectBig.top)*1.2);
    PolygonRotateShift(pt, 2, Orig.x, Orig.y,
		       bearing-DisplayAngle);
  }
  DrawDashLine(hdc, 2, pt[0], pt[1], RGB(0,0,0), rc);
}


void MapWindow::CalculateScreenPositionsTask() {
  unsigned int i;

  LockTaskData();

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

