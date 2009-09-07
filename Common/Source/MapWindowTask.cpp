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
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "InfoBoxLayout.h"
#include "AATDistance.h"
#include "Math/FastMath.h"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "Compatibility/gdi.h"
#include "options.h" /* for IBLSCALE() */

#include <math.h>

void
MapWindow::DrawAbortedTask(Canvas &canvas)
{
  int i;
  if (!WayPointList) return;

  ScopeLock scopeLock(mutexTaskData); // protect from extrnal task changes
  Pen dash_pen(Pen::DASH, IBLSCALE(1), MapGfx.TaskColor);
  canvas.select(dash_pen);

  for(i = 0; i < MAXTASKPOINTS - 1; i++) {
    int index = Task[i].Index;
    if (ValidWayPoint(index))
      canvas.line(WayPointList[index].Screen, Orig_Aircraft);
  }
}


void
MapWindow::DrawStartSector(Canvas &canvas, POINT &Start, POINT &End, int Index)
{
  if(StartLine) {
    canvas.select(MapGfx.hpStartFinishThick);
    canvas.line(WayPointList[Index].Screen, Start);
    canvas.line(WayPointList[Index].Screen, End);
    canvas.select(MapGfx.hpStartFinishThin);
    canvas.line(WayPointList[Index].Screen, Start);
    canvas.line(WayPointList[Index].Screen, End);
  } else {
    unsigned tmp = DistanceMetersToScreen(StartRadius);
    canvas.hollow_brush();
    canvas.select(MapGfx.hpStartFinishThick);
    canvas.circle(WayPointList[Index].Screen.x, WayPointList[Index].Screen.y,
                  tmp);
    canvas.select(MapGfx.hpStartFinishThin);
    canvas.circle(WayPointList[Index].Screen.x, WayPointList[Index].Screen.y,
                  tmp);
  }

}


void MapWindow::DrawTask(Canvas &canvas, RECT rc)
{
  int i;

  if (!WayPointList) return;
  Pen pent1(Pen::SOLID, IBLSCALE(1), MapGfx.TaskColor);
  Pen penb2(Pen::SOLID, IBLSCALE(2), Color(0,0,255));

  ScopeLock scopeLock(mutexTaskData); // protect from extrnal task changes

  if (ValidTaskPoint(0) && ValidTaskPoint(1) && (ActiveWayPoint<2)) {
    DrawStartSector(canvas, Task[0].Start, Task[0].End, Task[0].Index);
    if (EnableMultipleStartPoints) {
      for (i=0; i<MAXSTARTPOINTS; i++) {
        if (StartPoints[i].Active && ValidWayPoint(StartPoints[i].Index)) {
          DrawStartSector(canvas,
                          StartPoints[i].Start,
                          StartPoints[i].End, StartPoints[i].Index);
        }
      }
    }
  }

  Pen dash_pen5(Pen::DASH, IBLSCALE(5), MapGfx.TaskColor);

  for(i=1;i<MAXTASKPOINTS-1;i++) {

    if(ValidTaskPoint(i) && !ValidTaskPoint(i+1)) { // final waypoint
      if (ActiveWayPoint>1) {
        // only draw finish line when past the first
        // waypoint.
        if(FinishLine) {
          canvas.select(dash_pen5);
          canvas.two_lines(Task[i].Start, WayPointList[Task[i].Index].Screen,
                           Task[i].End);
          canvas.select(MapGfx.hpStartFinishThin);
          canvas.two_lines(Task[i].Start, WayPointList[Task[i].Index].Screen,
                           Task[i].End);
        } else {
          unsigned tmp = DistanceMetersToScreen(FinishRadius);
          canvas.hollow_brush();
          canvas.select(MapGfx.hpStartFinishThick);
          canvas.circle(WayPointList[Task[i].Index].Screen.x,
                        WayPointList[Task[i].Index].Screen.y,
                        tmp);
          canvas.select(MapGfx.hpStartFinishThin);
          canvas.circle(WayPointList[Task[i].Index].Screen.x,
                        WayPointList[Task[i].Index].Screen.y,
                        tmp);
        }
      }
    }

    if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) { // normal sector
      if(AATEnabled != TRUE) {
        Pen dash_pen2(Pen::DASH, IBLSCALE(2), Color(127, 127, 127));
        canvas.select(dash_pen2);
        canvas.two_lines(Task[i].Start, WayPointList[Task[i].Index].Screen,
                         Task[i].End);

        canvas.hollow_brush();
        canvas.black_pen();
        if(SectorType== 0) {
          unsigned tmp = DistanceMetersToScreen(SectorRadius);
          canvas.circle(WayPointList[Task[i].Index].Screen.x,
                        WayPointList[Task[i].Index].Screen.y,
                        tmp);
        }
        if(SectorType==1) {
          unsigned tmp = DistanceMetersToScreen(SectorRadius);
          canvas.segment(WayPointList[Task[i].Index].Screen.x,
                         WayPointList[Task[i].Index].Screen.y,tmp, rc,
                         Task[i].AATStartRadial-DisplayAngle,
                         Task[i].AATFinishRadial-DisplayAngle);
        }
        if(SectorType== 2) {
          // JMW added german rules
          unsigned tmp = DistanceMetersToScreen(500);
          canvas.circle(WayPointList[Task[i].Index].Screen.x,
                        WayPointList[Task[i].Index].Screen.y,
                        tmp);

          tmp = DistanceMetersToScreen(10000);
          canvas.segment(WayPointList[Task[i].Index].Screen.x,
                         WayPointList[Task[i].Index].Screen.y, tmp, rc,
                         Task[i].AATStartRadial-DisplayAngle,
                         Task[i].AATFinishRadial-DisplayAngle);
        }
      } else {
        // JMW added iso lines
        if ((i==ActiveWayPoint) 
	    || (SettingsMap().TargetPan 
		&& (i==SettingsMap().TargetPanIndex))) {
          // JMW 20080616 flash arc line if very close to target
          static bool flip = false;

          if (Calculated().WaypointDistance<200.0) { // JMW hardcoded AATCloseDistance
            flip = !flip;
          } else {
            flip = true;
          }
          if (flip) {
            for (int j=0; j<MAXISOLINES-1; j++) {
              if (TaskStats[i].IsoLine_valid[j]
                  && TaskStats[i].IsoLine_valid[j+1]) {
                canvas.select(penb2);
                canvas.line(TaskStats[i].IsoLine_Screen[j],
                            TaskStats[i].IsoLine_Screen[j + 1]);
              }
            }
          }
        }
      }
    }
  }

  Pen dash_pen3(Pen::DASH, IBLSCALE(3), MapGfx.TaskColor);

  for(i=0;i<MAXTASKPOINTS-1;i++) {
    if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
      bool is_first = (Task[i].Index < Task[i+1].Index);
      int imin = min(Task[i].Index,Task[i+1].Index);
      int imax = max(Task[i].Index,Task[i+1].Index);
      // JMW AAT!
      double bearing = Task[i].OutBound;
      POINT sct1, sct2;

      canvas.select(dash_pen3);

      if (AATEnabled && !SettingsMap().TargetPan) {
        LonLat2Screen(Task[i].AATTargetLon,
                      Task[i].AATTargetLat,
                      sct1);
        LonLat2Screen(Task[i+1].AATTargetLon,
                      Task[i+1].AATTargetLat,
                      sct2);
        DistanceBearing(Task[i].AATTargetLat,
                        Task[i].AATTargetLon,
                        Task[i+1].AATTargetLat,
                        Task[i+1].AATTargetLon,
                        NULL, &bearing);

        // draw nominal track line
        canvas.line(WayPointList[imin].Screen, WayPointList[imax].Screen);
      } else {
        sct1 = WayPointList[Task[i].Index].Screen;
        sct2 = WayPointList[Task[i+1].Index].Screen;
      }

      if (is_first) {
        canvas.line(sct1, sct2);
      } else {
        canvas.line(sct2, sct1);
      }

      // draw small arrow along task direction
      POINT p_p;
      POINT Arrow[3] = { {6,6}, {-6,6}, {0,0} };
      ScreenClosestPoint(sct1, sct2,
                         Orig_Aircraft, &p_p, IBLSCALE(25));
      PolygonRotateShift(Arrow, 2, p_p.x, p_p.y,
                         bearing-DisplayAngle);
      Arrow[2] = Arrow[1];
      Arrow[1] = p_p;

      canvas.select(pent1);
      canvas.polyline(Arrow, 3);
    }
  }
}


void MapWindow::DrawTaskAAT(Canvas &canvas, const RECT rc, Canvas &buffer)
{
  int i;
  unsigned tmp;

  if (!WayPointList) return;
  if (!AATEnabled) return;

  ScopeLock scopeLock(mutexTaskData); // protect from extrnal task changes

  Color whitecolor = Color(0xff,0xff, 0xff);
  buffer.set_text_color(whitecolor);
  buffer.white_pen();
  buffer.white_brush();
  buffer.rectangle(rc.left, rc.top, rc.right, rc.bottom);

  for (i = MAXTASKPOINTS - 2; i > 0; i--) {
    if(ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
      if(Task[i].AATType == CIRCLE) {
        tmp = DistanceMetersToScreen(Task[i].AATCircleRadius);

        // this color is used as the black bit
        buffer.set_text_color(MapGfx.Colours[iAirspaceColour[AATASK]]);

        // this color is the transparent bit
        buffer.set_background_color(whitecolor);

        if (i<ActiveWayPoint) {
          buffer.hollow_brush();
        } else {
          buffer.select(MapGfx.hAirspaceBrushes[iAirspaceBrush[AATASK]]);
        }
        buffer.black_pen();

        buffer.circle(WayPointList[Task[i].Index].Screen.x,
                      WayPointList[Task[i].Index].Screen.y,
                      tmp);
      } else {

        // this color is used as the black bit
        buffer.set_text_color(MapGfx.Colours[iAirspaceColour[AATASK]]);

        // this color is the transparent bit
        buffer.set_background_color(whitecolor);

        if (i<ActiveWayPoint) {
          buffer.hollow_brush();
        } else {
          buffer.select(MapGfx.hAirspaceBrushes[iAirspaceBrush[AATASK]]);
        }
        buffer.black_pen();

        tmp = DistanceMetersToScreen(Task[i].AATSectorRadius);

        buffer.segment(WayPointList[Task[i].Index].Screen.x,
                       WayPointList[Task[i].Index].Screen.y, tmp, rc,
                       Task[i].AATStartRadial-DisplayAngle,
                       Task[i].AATFinishRadial-DisplayAngle);

        buffer.two_lines(Task[i].AATStart, WayPointList[Task[i].Index].Screen,
                         Task[i].AATFinish);
      }

    }
  }

  canvas.copy_transparent_white(buffer, rc);
}


void MapWindow::DrawBearing(Canvas &canvas, int bBearingValid)
{ /* RLD bearing is invalid if GPS not connected and in non-sim mode,
   but we can still draw targets */

  if (!ValidTaskPoint(ActiveWayPoint)) {
    return;
  }

  mutexTaskData.Lock();  // protect from extrnal task changes

  double startLat = Basic().Latitude;
  double startLon = Basic().Longitude;
  double targetLat;
  double targetLon;

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)) {
    targetLat = Task[ActiveWayPoint].AATTargetLat;
    targetLon = Task[ActiveWayPoint].AATTargetLon;
  } else {
    targetLat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    targetLon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
  }
  mutexTaskData.Unlock();
  if (bBearingValid) {
      DrawGreatCircle(canvas, startLon, startLat,  // RLD skip if bearing invalid
		      targetLon, targetLat);// RLD bc Lat/Lon invalid

    if (SettingsMap().TargetPan) {
      // Draw all of task if in target pan mode
      startLat = targetLat;
      startLon = targetLon;

      ScopeLock scopeLock(mutexTaskData);

      for (int i=ActiveWayPoint+1; i<MAXTASKPOINTS; i++) {
        if (ValidTaskPoint(i)) {

          if (AATEnabled && ValidTaskPoint(i+1)) {
            targetLat = Task[i].AATTargetLat;
            targetLon = Task[i].AATTargetLon;
          } else {
            targetLat = WayPointList[Task[i].Index].Latitude;
            targetLon = WayPointList[Task[i].Index].Longitude;
          }

          DrawGreatCircle(canvas, startLon, startLat,
                          targetLon, targetLat);

          startLat = targetLat;
          startLon = targetLon;
        }
      }
    } // TargetPan
  } // bearing valid

  // JMW draw symbol at target, makes it easier to see
  // RLD always draw all targets ahead so visible in pan mode
  if (AATEnabled) {
    ScopeLock scopeLock(mutexTaskData);

    for (int i=ActiveWayPoint; i<MAXTASKPOINTS; i++) {
      // RLD skip invalid targets and targets at start and finish
      if((i>0) && ValidTaskPoint(i) && ValidTaskPoint(i+1)) {
        if (i>= ActiveWayPoint) {
	  draw_masked_bitmap_if_visible(canvas, MapGfx.hBmpTarget, 
					Task[i].AATTargetLon,
					Task[i].AATTargetLat, 
					10, 10);
        }
      }
    }
  }

}



void
MapWindow::DrawOffTrackIndicator(Canvas &canvas)
{
  if ((ActiveWayPoint<=0) || !ValidTaskPoint(ActiveWayPoint)) {
    return;
  }
  if (fabs(Basic().TrackBearing-Calculated().WaypointBearing)<10) {
    // insignificant error
    return;
  }
  if (Calculated().Circling || TaskIsTemporary() || 
      SettingsMap().TargetPan) {
    // don't display in various modes
    return;
  }

  double distance_max = min(Calculated().WaypointDistance,
			    GetScreenDistanceMeters()*0.7);
  if (distance_max < 5000.0) {
    // too short to bother
    return;
  }

  mutexTaskData.Lock();  // protect from extrnal task changes

  double startLat = Basic().Latitude;
  double startLon = Basic().Longitude;
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
  mutexTaskData.Unlock();

  canvas.select(TitleWindowFont);
  canvas.set_text_color(Color(0x0, 0x0, 0x0));

  int ilast = 0;
  for (double d=0.25; d<=1.0; d+= 0.25) {
    double distance0, distance1;

    FindLatitudeLongitude(startLat, startLon,
			  Basic().TrackBearing,
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

    double distance = (distance0+distance1)/Calculated().WaypointDistance;
    int idist = iround((distance-1.0)*100);

    if ((idist != ilast) && (idist>0) && (idist<1000)) {

      TCHAR Buffer[5];
      _stprintf(Buffer, TEXT("%d"), idist);
      POINT sc;
      RECT brect;
      LonLat2Screen(dLon, dLat, sc);
      SIZE tsize = canvas.text_size(Buffer);

      brect.left = sc.x-4;
      brect.right = brect.left+tsize.cx+4;
      brect.top = sc.y-4;
      brect.bottom = brect.top+tsize.cy+4;

      if (label_block.check(brect)) {
        canvas.text(sc.x - tsize.cx / 2, sc.y - tsize.cy / 2, Buffer);
	ilast = idist;
      }
    }

  }
}



void
MapWindow::DrawProjectedTrack(Canvas &canvas)
{
  if ((ActiveWayPoint<=0) || !ValidTaskPoint(ActiveWayPoint) || !AATEnabled) {
    return;
  }
  if (Calculated().Circling || TaskIsTemporary()) {
    // don't display in various modes
    return;
  }

  // TODO feature: maybe have this work even if no task?
  // TODO feature: draw this also when in target pan mode

  mutexTaskData.Lock();  // protect from extrnal task changes

  double startLat = Basic().Latitude;
  double startLon = Basic().Longitude;
  double previousLat;
  double previousLon;
  if (AATEnabled) {
    previousLat = Task[max(0,ActiveWayPoint-1)].AATTargetLat;
    previousLon = Task[max(0,ActiveWayPoint-1)].AATTargetLon;
  } else {
    previousLat = WayPointList[Task[max(0,ActiveWayPoint-1)].Index].Latitude;
    previousLon = WayPointList[Task[max(0,ActiveWayPoint-1)].Index].Longitude;
  }
  mutexTaskData.Unlock();

  double distance_from_previous, bearing;
  DistanceBearing(previousLat, previousLon,
		  startLat, startLon,
		  &distance_from_previous,
		  &bearing);

  if (distance_from_previous < 100.0) {
    bearing = Basic().TrackBearing;
    // too short to have valid data
  }
  POINT pt[2] = {{0,-75},{0,-400}};
  if (SettingsMap().TargetPan) {
    double screen_range = GetScreenDistanceMeters();
    double flow = 0.4;
    double fhigh = 1.5;
    screen_range = max(screen_range, Calculated().WaypointDistance);

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
    LonLat2Screen(p1Lon, p1Lat, pt[0]);
    LonLat2Screen(p2Lon, p2Lat, pt[1]);
  } else if (fabs(bearing-Calculated().WaypointBearing)<10) {
    // too small an error to bother
    return;
  } else {
    pt[1].y = (long)(-max(MapRectBig.right-MapRectBig.left,
			  MapRectBig.bottom-MapRectBig.top)*1.2);
    PolygonRotateShift(pt, 2, Orig_Aircraft.x, Orig_Aircraft.y,
		       bearing-DisplayAngle);
  }

  Pen dash_pen(Pen::DASH, IBLSCALE(2), Color(0, 0, 0));
  canvas.select(dash_pen);
  canvas.line(pt[0], pt[1]);
}


void MapWindow::CalculateScreenPositionsTask() {
  unsigned int i;

  ScopeLock scopeLock(mutexTaskData);

  if (EnableMultipleStartPoints) {
    for(i=0;i<MAXSTARTPOINTS-1;i++) {
      if (StartPoints[i].Active && ValidWayPoint(StartPoints[i].Index)) {
        LonLat2Screen(StartPoints[i].SectorEndLon,
                      StartPoints[i].SectorEndLat, StartPoints[i].End);
        LonLat2Screen(StartPoints[i].SectorStartLon,
                      StartPoints[i].SectorStartLat, StartPoints[i].Start);
      }
    }
  }

  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    bool this_valid = ValidTaskPoint(i);
    bool next_valid = ValidTaskPoint(i+1);
    if (AATEnabled && this_valid) {
      LonLat2Screen(Task[i].AATTargetLon, Task[i].AATTargetLat,
                    Task[i].Target);
    }

    if(this_valid && !next_valid)
    {
      // finish
      LonLat2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, Task[i].End);
      LonLat2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, Task[i].Start);
    }
    if(this_valid && next_valid)
    {
      LonLat2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, Task[i].End);
      LonLat2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, Task[i].Start);

      if((AATEnabled) && (Task[i].AATType == SECTOR))
      {
        LonLat2Screen(Task[i].AATStartLon, Task[i].AATStartLat, Task[i].AATStart);
        LonLat2Screen(Task[i].AATFinishLon, Task[i].AATFinishLat, Task[i].AATFinish);
      }
      if (AATEnabled && (((int)i==ActiveWayPoint) ||
			 (SettingsMap().TargetPan 
			  && ((int)i==SettingsMap().TargetPanIndex)))) {

	for (int j=0; j<MAXISOLINES; j++) {
	  if (TaskStats[i].IsoLine_valid[j]) {
	    LonLat2Screen(TaskStats[i].IsoLine_Longitude[j],
			  TaskStats[i].IsoLine_Latitude[j],
			  TaskStats[i].IsoLine_Screen[j]);
	  }
	}
      }
    }
  }
}

