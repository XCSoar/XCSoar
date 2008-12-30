/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

#include "StdAfx.h"
#include "compatibility.h"
#include "MapWindow.h"
#include "OnLineContest.h"
#include "Utils.h"
#include "Units.h"
#include "Logger.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"
#include "Dialogs.h"
#include "externs.h"
#include "VarioSound.h"
#include "InputEvents.h"
// #include <assert.h>
#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "Terrain.h"
#include "options.h"
#include "Task.h"

#include "GaugeVarioAltA.h"
#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

#ifdef DEBUG
#if (WINDOWSPC<1)
#define DRAWLOAD
#endif
#endif

extern HWND hWndCDIWindow;
extern HFONT TitleSmallWindowFont;


void MapWindow::DrawCDI() {
  bool dodrawcdi = false;

  if (DerivedDrawInfo.Circling) {
    if (EnableCDICircling) {
      dodrawcdi = true;
    }
  } else {
    if (EnableCDICruise) {
      dodrawcdi = true;
    }
  }

  if (dodrawcdi) {
    ShowWindow(hWndCDIWindow, SW_SHOW);

    // JMW changed layout here to fit reorganised display
    // insert waypoint bearing ".<|>." into CDIScale string"

    TCHAR CDIScale[] = TEXT("330..340..350..000..010..020..030..040..050..060..070..080..090..100..110..120..130..140..150..160..170..180..190..200..210..220..230..240..250..260..270..280..290..300..310..320..330..340..350..000..010..020..030..040.");
    TCHAR CDIDisplay[25] = TEXT("");
    int j;
    int CDI_WP_Bearing = (int)DerivedDrawInfo.WaypointBearing/2;
    CDIScale[CDI_WP_Bearing + 9] = 46;
    CDIScale[CDI_WP_Bearing + 10] = 60;
    CDIScale[CDI_WP_Bearing + 11] = 124; // "|" character
    CDIScale[CDI_WP_Bearing + 12] = 62;
    CDIScale[CDI_WP_Bearing + 13] = 46;
    for (j=0;j<24;j++) CDIDisplay[j] = CDIScale[(j + (int)(DrawInfo.TrackBearing)/2)];
    CDIDisplay[24] = _T('\0');
    // JMW fix bug! This indicator doesn't always display correctly!

    // JMW added arrows at end of CDI to point to track if way off..
    int deltacdi = iround(DerivedDrawInfo.WaypointBearing-DrawInfo.TrackBearing);

    while (deltacdi>180) {
      deltacdi-= 360;
    }
    while (deltacdi<-180) {
      deltacdi+= 360;
    }
    if (deltacdi>20) {
      CDIDisplay[21]='>';
      CDIDisplay[22]='>';
      CDIDisplay[23]='>';
    }
    if (deltacdi<-20) {
      CDIDisplay[0]='<';
      CDIDisplay[1]='<';
      CDIDisplay[2]='<';
    }

    SetWindowText(hWndCDIWindow,CDIDisplay);
    // end of new code to display CDI scale
  } else {
    ShowWindow(hWndCDIWindow, SW_HIDE);
  }
}



double MapWindow::findMapScaleBarSize(RECT rc) {

  int range = rc.bottom-rc.top;
//  int nbars = 0;
//  int nscale = 1;
  double pixelsize = MapScale/GetMapResolutionFactor(); // km/pixel

  // find largest bar size that will fit in display

  double displaysize = range*pixelsize/2; // km

  if (displaysize>100.0) {
    return 100.0/pixelsize;
  }
  if (displaysize>10.0) {
    return 10.0/pixelsize;
  }
  if (displaysize>1.0) {
    return 1.0/pixelsize;
  }
  if (displaysize>0.1) {
    return 0.1/pixelsize;
  }
  // this is as far as is reasonable
  return 0.1/pixelsize;
}


void MapWindow::DrawMapScale2(HDC hDC, RECT rc, POINT Orig_Aircraft)
{

  if (Appearance.MapScale2 == apMs2None) return;

  HPEN hpOld   = (HPEN)SelectObject(hDC, hpMapScale);
  HPEN hpWhite = (HPEN)GetStockObject(WHITE_PEN);
  HPEN hpBlack = (HPEN)GetStockObject(BLACK_PEN);

  bool color = false;
  POINT Start, End={0,0};
  bool first=true;

  int barsize = iround(findMapScaleBarSize(rc));

  Start.x = rc.right-1;
  for (Start.y=Orig_Aircraft.y; Start.y<rc.bottom+barsize; Start.y+= barsize) {
    if (color) {
      SelectObject(hDC, hpWhite);
    } else {
      SelectObject(hDC, hpBlack);
    }
    if (!first) {
      DrawSolidLine(hDC,Start,End);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }

  color = true;
  first = true;
  for (Start.y=Orig_Aircraft.y; Start.y>rc.top-barsize; Start.y-= barsize) {
    if (color) {
      SelectObject(hDC, hpWhite);
    } else {
      SelectObject(hDC, hpBlack);
    }
    if (!first) {
      DrawSolidLine(hDC,Start,End);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }

  // draw text as before

  SelectObject(hDC, hpOld);

}


/*
void MapWindow::DrawSpeedToFly(HDC hDC, RECT rc) {
  POINT chevron[3];

  HPEN hpOld;
  HBRUSH hbOld;

  //  TCHAR Value[10];
  int i;

  if (Appearance.DontShowSpeedToFly || !DerivedDrawInfo.Flying)
    return;

#ifndef _SIM_
  if (!(DrawInfo.AirspeedAvailable && DrawInfo.VarioAvailable)) {
    return;
  }
#else
  // cheat
  DrawInfo.IndicatedAirspeed = DrawInfo.Speed;
#endif

  hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
  hpOld = (HPEN)SelectObject(hDC, hpBearing);

  double vdiff;
  int vsize = (rc.bottom-rc.top)/2;

  vdiff = (DerivedDrawInfo.VOpt - DrawInfo.IndicatedAirspeed)/40.0;
  // 25.0 m/s is maximum scale
  vdiff = max(-0.5,min(0.5,vdiff)); // limit it

  int yoffset=0;
  int hyoffset=0;
  vsize = iround(fabs(vdiff*vsize));
  int xoffset = rc.right-IBLSCALE(25);
  int ycenter = (rc.bottom+rc.top)/2;

  int k=0;

  for (k=0; k<2; k++) {

    for (i=0; i< vsize; i+= 5) {
      if (vdiff>0) {
        yoffset = i+ycenter+k;
        hyoffset = IBLSCALE(4);
      } else {
        yoffset = -i+ycenter-k;
        hyoffset = -IBLSCALE(4);
      }
      chevron[0].x = xoffset;
      chevron[0].y = yoffset;
      chevron[1].x = xoffset+IBLSCALE(10);
      chevron[1].y = yoffset+hyoffset;
      chevron[2].x = xoffset+IBLSCALE(20);
      chevron[2].y = yoffset;

      Polyline(hDC, chevron, 3);
    }
    if (vdiff>0) {
      hpOld = (HPEN)SelectObject(hDC, hpSpeedSlow);
    } else {
      hpOld = (HPEN)SelectObject(hDC, hpSpeedFast);
    }
  }

  SelectObject(hDC, hpBearing);
  chevron[0].x = xoffset-IBLSCALE(3);
  chevron[0].y = ycenter;
  chevron[1].x = xoffset+IBLSCALE(3+20);
  chevron[1].y = ycenter;
  Polyline(hDC, chevron, 2);

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}
*/

#define fSnailColour(cv) max(0,min((short)(NUMSNAILCOLORS-1), (short)((cv+1.0)/2.0*NUMSNAILCOLORS)))

void MapWindow::DrawFLARMTraffic(HDC hDC, RECT rc) {

  if (!EnableFLARMDisplay) return;

  if (!DrawInfo.FLARM_Available) return;

  HPEN hpOld;
  POINT Arrow[5];

  hpOld = (HPEN)SelectObject(hDC, hpBestCruiseTrack);

  int i;
//  double dX, dY;
  TextInBoxMode_t displaymode;
  displaymode.AsInt = 0;

  double screenrange = GetApproxScreenRange();
  double scalefact = screenrange/6000.0;

  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (DrawInfo.FLARM_Traffic[i].ID!=0) {

      double target_lon;
      double target_lat;

      target_lon = DrawInfo.FLARM_Traffic[i].Longitude;
      target_lat = DrawInfo.FLARM_Traffic[i].Latitude;

      if ((EnableFLARMDisplay==2)&&(scalefact>1.0)) {
        double distance;
        double bearing;

        DistanceBearing(DrawInfo.Latitude,
                        DrawInfo.Longitude,
                        target_lat,
                        target_lon,
                        &distance,
                        &bearing);

        FindLatitudeLongitude(DrawInfo.Latitude,
                              DrawInfo.Longitude,
                              bearing,
                              distance*scalefact,
                              &target_lat,
                              &target_lon);

      }

      // TODO: draw direction, height?
      POINT sc;
      LatLon2Screen(target_lon,
                    target_lat,
                    sc);

#ifndef FLARM_AVERAGE
      if (DrawInfo.FLARM_Traffic[i].Name) {
        TextInBox(hDC, DrawInfo.FLARM_Traffic[i].Name, sc.x+IBLSCALE(3),
                  sc.y, 0, displaymode,
                  true);
      }

      DrawBitmapIn(hDC, sc, hFLARMTraffic);
#else
      TCHAR label1[100];
      TCHAR label2[100];
      if (DrawInfo.FLARM_Traffic[i].Name)
	{
	  wsprintf(label1, TEXT("%s"), DrawInfo.FLARM_Traffic[i].Name);
	  if (DrawInfo.FLARM_Traffic[i].Average30s>0) {
	    wsprintf(label2, TEXT("%.1lf"), DrawInfo.FLARM_Traffic[i].Average30s);
	  } else {
	    label2[0]= _T('\0');
	  }
	}
      else
	{
	  label1[0]= _T('\0');
	  if (DrawInfo.FLARM_Traffic[i].Average30s>0) {
	    wsprintf(label2, TEXT("%.1lf"), DrawInfo.FLARM_Traffic[i].Average30s);
	  } else {
	    label2[0]= _T('\0');
	  }
	}

      float vmax = (float)(1.5*min(5.0, max(MACCREADY,0.5)));
      float vmin = (float)(-1.5*min(5.0, max(MACCREADY,2.0)));

      float cv = DrawInfo.FLARM_Traffic[i].Average30s;
      if (cv<0) {
        cv /= (-vmin); // JMW fixed bug here
      } else {
        cv /= vmax;
      }

      int colourIndex = fSnailColour(cv);

      HGDIOBJ oldFont = SelectObject(hDC, TitleSmallWindowFont);
      COLORREF oldTextColor = SetTextColor(hDC, hSnailColours[colourIndex]);

      if (wcslen(label2)>0) {
	ExtTextOut(hDC, sc.x+IBLSCALE(3),sc.y+IBLSCALE(8), ETO_OPAQUE, NULL, label2,
		   wcslen(label2), NULL);
      }
      SetTextColor(hDC, RGB(255,0,0));
      if (wcslen(label1)>0) {
	ExtTextOut(hDC, sc.x+IBLSCALE(3),sc.y, ETO_OPAQUE, NULL, label1, wcslen(label1), NULL);
      }

      SelectObject(hDC, oldFont);
      SetTextColor(hDC, oldTextColor);

      DrawBitmapIn(hDC, sc, hFLARMTraffic);
#endif

      Arrow[0].x = -4;
      Arrow[0].y = 5;
      Arrow[1].x = 0;
      Arrow[1].y = -6;
      Arrow[2].x = 4;
      Arrow[2].y = 5;
      Arrow[3].x = 0;
      Arrow[3].y = 2;
      Arrow[4].x = -4;
      Arrow[4].y = 5;

      //      double vmag = max(1.0,min(15.0,DrawInfo.FLARM_Traffic[i].Speed/5.0))*2;

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         DrawInfo.FLARM_Traffic[i].TrackBearing - DisplayAngle);
      Polygon(hDC,Arrow,5);

    }
  }

  SelectObject(hDC, hpOld);

}


//////////////////////
// JMW added simple code to prevent text writing over map city names
int MapWindow::nLabelBlocks;
RECT MapWindow::LabelBlockCoords[MAXLABELBLOCKS];

bool MapWindow::checkLabelBlock(RECT rc) {
  bool ok = true;

  for (int i=0; i<nLabelBlocks; i++) {
    if (CheckRectOverlap(LabelBlockCoords[i],rc)) {
      ok = false;
      continue;
    }
  }
  if (nLabelBlocks<MAXLABELBLOCKS-1) {
    LabelBlockCoords[nLabelBlocks]= rc;
    nLabelBlocks++;
  }
  return ok;
}


rectObj MapWindow::CalculateScreenBounds(double scale) {
  // compute lat lon extents of visible screen
  rectObj sb;

  if (scale>= 1.0) {
    POINT screen_center;
    LatLon2Screen(PanLongitude,
                  PanLatitude,
                  screen_center);

    sb.minx = sb.maxx = PanLongitude;
    sb.miny = sb.maxy = PanLatitude;

    int dx, dy;
    unsigned int maxsc=0;
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.left;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-MapRect.right;
    dy = screen_center.y-MapRect.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));

    for (int i=0; i<10; i++) {
      double ang = i*360.0/10;
      POINT p;
      double X, Y;
      p.x = screen_center.x + iround(fastcosine(ang)*maxsc*scale);
      p.y = screen_center.y + iround(fastsine(ang)*maxsc*scale);
      Screen2LatLon(p.x, p.y, X, Y);
      sb.minx = min(X, sb.minx);
      sb.miny = min(Y, sb.miny);
      sb.maxx = max(X, sb.maxx);
      sb.maxy = max(Y, sb.maxy);
    }

  } else {

    double xmin, xmax, ymin, ymax;
    int x, y;
    double X, Y;

    x = MapRect.left;
    y = MapRect.top;
    Screen2LatLon(x, y, X, Y);
    xmin = X; xmax = X;
    ymin = Y; ymax = Y;

    x = MapRect.right;
    y = MapRect.top;
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);

    x = MapRect.right;
    y = MapRect.bottom;
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);

    x = MapRect.left;
    y = MapRect.bottom;
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);


    sb.minx = xmin;
    sb.maxx = xmax;
    sb.miny = ymin;
    sb.maxy = ymax;

  }

  return sb;
}



void MapWindow::ScanVisibility(rectObj *bounds_active) {
  // received when the SetTopoBounds determines the visibility
  // boundary has changed.
  // This happens rarely, so it is good pre-filtering of what is visible.
  // (saves from having to do it every screen redraw)
  const rectObj bounds = *bounds_active;

  // far visibility for snail trail

  SNAIL_POINT *sv= SnailTrail;
  const SNAIL_POINT *se = sv+TRAILSIZE;
  while (sv<se) {
    sv->FarVisible = ((sv->Longitude> bounds.minx) &&
		      (sv->Longitude< bounds.maxx) &&
		      (sv->Latitude> bounds.miny) &&
		      (sv->Latitude< bounds.maxy));
    sv++;
  }

  // far visibility for waypoints

  if (WayPointList) {
    WAYPOINT *wv = WayPointList;
    const WAYPOINT *we = WayPointList+NumberOfWayPoints;
    while (wv<we) {
      // TODO optimise
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


void MapWindow::CalculateScreenPositionsThermalSources() {
  for (int i=0; i<MAX_THERMAL_SOURCES; i++) {
    if (DerivedDrawInfo.ThermalSources[i].LiftRate>0) {
      double dh = DerivedDrawInfo.NavAltitude
        -DerivedDrawInfo.ThermalSources[i].GroundHeight;
      if (dh<0) {
        DerivedDrawInfo.ThermalSources[i].Visible = false;
        continue;
      }

      double t = dh/DerivedDrawInfo.ThermalSources[i].LiftRate;
      double lat, lon;
      FindLatitudeLongitude(DerivedDrawInfo.ThermalSources[i].Latitude,
                            DerivedDrawInfo.ThermalSources[i].Longitude,
                            DerivedDrawInfo.WindBearing,
                            -DerivedDrawInfo.WindSpeed*t,
                            &lat, &lon);
      if (PointVisible(lon,lat)) {
        LatLon2Screen(lon,
                      lat,
                      DerivedDrawInfo.ThermalSources[i].Screen);
        DerivedDrawInfo.ThermalSources[i].Visible =
          PointVisible(DerivedDrawInfo.ThermalSources[i].Screen);
      } else {
        DerivedDrawInfo.ThermalSources[i].Visible = false;
      }
    } else {
      DerivedDrawInfo.ThermalSources[i].Visible = false;
    }
  }
}


void MapWindow::CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE &circ) {
  circ.Visible = false;
  if (!circ.FarVisible) return;
  if (iAirspaceMode[circ.Type]%2 == 1)
    if(CheckAirspaceAltitude(circ.Base.Altitude,
                             circ.Top.Altitude)) {
      if (msRectOverlap(&circ.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &circ.bounds)) {

	if (!circ._NewWarnAckNoBrush &&
	    !(iAirspaceBrush[circ.Type] == NUMAIRSPACEBRUSHES-1)) {
	  circ.Visible = 2;
	} else {
	  circ.Visible = 1;
	}

        LatLon2Screen(circ.Longitude,
                      circ.Latitude,
                      circ.Screen);
        circ.ScreenR = iround(circ.Radius*ResMapScaleOverDistanceModify);
      }
    }
}

void MapWindow::CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA &area) {
  area.Visible = false;
  if (!area.FarVisible) return;
  if (iAirspaceMode[area.Type]%2 == 1)
    if(CheckAirspaceAltitude(area.Base.Altitude,
                             area.Top.Altitude)) {
      if (msRectOverlap(&area.bounds, &screenbounds_latlon)
          || msRectContained(&screenbounds_latlon, &area.bounds)) {
        AIRSPACE_POINT *ap= AirspacePoint+area.FirstPoint;
        const AIRSPACE_POINT *ep= ap+area.NumPoints;
        POINT* sp= AirspaceScreenPoint+area.FirstPoint;
        while (ap < ep) {
	  // JMW optimise!
            LatLon2Screen(ap->Longitude,
                          ap->Latitude,
                          *sp);
            ap++;
            sp++;
        }

	if (!area._NewWarnAckNoBrush &&
	    !(iAirspaceBrush[area.Type] == NUMAIRSPACEBRUSHES-1)) {
	  area.Visible = 2;
	} else {
	  area.Visible = 1;
	}
      }
    }
}

void MapWindow::CalculateScreenPositionsAirspace() {


  if (AirspaceCircle) {
    for (AIRSPACE_CIRCLE* circ = AirspaceCircle;
         circ < AirspaceCircle+NumberOfAirspaceCircles; circ++) {
      CalculateScreenPositionsAirspaceCircle(*circ);
    }
  }
  if (AirspaceArea) {
    for(AIRSPACE_AREA *area = AirspaceArea;
        area < AirspaceArea+NumberOfAirspaceAreas; area++) {
      CalculateScreenPositionsAirspaceArea(*area);
    }
  }
}


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
        // JMW TODO: only pan if distance of center to aircraft is smaller
        // than one third screen width

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


void MapWindow::DrawHorizon(HDC hDC,RECT rc)
{
  POINT Start;

  Start.y = IBLSCALE(55)+rc.top;
  Start.x = rc.right - IBLSCALE(19);
  if (EnableVarioGauge && MapRectBig.right == rc.right)
    Start.x -= InfoBoxLayout::ControlWidth;

  HPEN   hpHorizonSky;
  HBRUSH hbHorizonSky;
  HPEN   hpHorizonGround;
  HBRUSH hbHorizonGround;
  HPEN   hpOld;
  HBRUSH hbOld;

  hpHorizonSky = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
                                 RGB(0x40,0x40,0xff));
  hbHorizonSky = (HBRUSH)CreateSolidBrush(RGB(0xA0,0xA0,0xff));

  hpHorizonGround = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
                                    RGB(106,55,12));
  hbHorizonGround = (HBRUSH)CreateSolidBrush(
                                             RGB(157,101,60));

  int radius = IBLSCALE(17);
  double phi = max(-89,min(89,DerivedDrawInfo.BankAngle));
  double alpha = RAD_TO_DEG
    *acos(max(-1.0,min(1.0,DerivedDrawInfo.PitchAngle/50.0)));
  double alpha1 = 180-alpha-phi;
  double alpha2 = 180+alpha-phi;

  hpOld = (HPEN)SelectObject(hDC, hpHorizonSky);
  hbOld = (HBRUSH)SelectObject(hDC, hbHorizonSky);

  Segment(hDC, Start.x, Start.y, radius, rc,
          alpha2, alpha1, true);

  SelectObject(hDC, hpHorizonGround);
  SelectObject(hDC, hbHorizonGround);

  Segment(hDC, Start.x, Start.y, radius, rc,
          alpha1, alpha2, true);

  POINT a1, a2;

  /*
  a1.x = Start.x + fastsine(alpha1)*radius;
  a1.y = Start.y - fastcosine(alpha1)*radius;
  a2.x = Start.x + fastsine(alpha2)*radius;
  a2.y = Start.y - fastcosine(alpha2)*radius;

  _DrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0));
  */

  a1.x = Start.x+radius/2;
  a1.y = Start.y;
  a2.x = Start.x-radius/2;
  a2.y = Start.y;
  _DrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, a2, RGB(0,0,0));

  a1.x = Start.x;
  a1.y = Start.y-radius/4;
  _DrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, Start, RGB(0,0,0));

  //

#define ROOT2 0.70711

  int rr2p = lround(radius*ROOT2+IBLSCALE(1));
  int rr2n = lround(radius*ROOT2);

  a1.x = Start.x+rr2p;
  a1.y = Start.y-rr2p;
  a2.x = Start.x+rr2n;
  a2.y = Start.y-rr2n;

  _DrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0));

  a1.x = Start.x-rr2p;
  a1.y = Start.y-rr2p;
  a2.x = Start.x-rr2n;
  a2.y = Start.y-rr2n;

  _DrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0));

  // JMW experimental, display stall sensor
  double s = max(0.0,min(1.0,DrawInfo.StallRatio));
  long m = (long)((rc.bottom-rc.top)*s*s);
  a1.x = rc.right-1;
  a1.y = rc.bottom-m;
  a2.x = a1.x-10;
  a2.y = a1.y;
  _DrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, a2, RGB(0xff,0,0));

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);
  DeleteObject((HPEN)hpHorizonSky);
  DeleteObject((HBRUSH)hbHorizonSky);
  DeleteObject((HPEN)hpHorizonGround);
  DeleteObject((HBRUSH)hbHorizonGround);
}


// JMW to be used for target preview
bool MapWindow::SetTargetPan(bool do_pan, int target_point) {
  static double old_latitude;
  static double old_longitude;
  static bool old_pan=false;
  static bool old_fullscreen=false;

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


void MapWindow::DrawGreatCircle(HDC hdc,
                                double startLon, double startLat,
                                double targetLon, double targetLat) {

  double distance=0;
  double distanceTotal=0;
  double Bearing;

  DistanceBearing(startLat,
                  startLon,
                  targetLat,
                  targetLon,
                  &distanceTotal,
                  &Bearing);

  distance = distanceTotal;

  if (distanceTotal==0.0) {
    return;
  }

  double d_distance = max(5000.0,distanceTotal/10);

  HPEN hpOld = (HPEN)SelectObject(hdc, hpBearing);

  POINT StartP;
  POINT EndP;
  LatLon2Screen(startLon,
                startLat,
                StartP);
  LatLon2Screen(targetLon,
                targetLat,
                EndP);

  if (d_distance>distanceTotal) {
    DrawSolidLine(hdc, StartP, EndP);
  } else {

    for (int i=0; i<= 10; i++) {

      double tlat1, tlon1;

      FindLatitudeLongitude(startLat,
                            startLon,
                            Bearing,
                            min(distance,d_distance),
                            &tlat1,
                            &tlon1);

      DistanceBearing(tlat1,
                      tlon1,
                      targetLat,
                      targetLon,
                      &distance,
                      &Bearing);

      LatLon2Screen(tlon1,
                    tlat1,
                    EndP);

      DrawSolidLine(hdc, StartP, EndP);

      StartP.x = EndP.x;
      StartP.y = EndP.y;

      startLat = tlat1;
      startLon = tlon1;

    }
  }
  SelectObject(hdc, hpOld);
}



int MapWindow::iSnailNext=0;

// This function is slow...
double MapWindow::DrawTrail( HDC hdc, const POINT Orig, const RECT rc)
{
  int i, j;
  SNAIL_POINT P1;
#ifdef NOLINETO
  SNAIL_POINT P2;
#endif
  static BOOL lastCircling = FALSE;
  static float vmax= 5.0;
  static float vmin= -5.0;
  static bool needcolour = true;
  bool first = true;
  double TrailFirstTime = -1;

  if(!TrailActive)
    return -1;

  if ((DisplayMode == dmCircling) != lastCircling) {
    needcolour = true;
  }
  lastCircling = (DisplayMode == dmCircling);

  double traildrift_lat = 0.0;
  double traildrift_lon = 0.0;

  const bool dotraildrift = EnableTrailDrift && (DisplayMode == dmCircling);

  if (dotraildrift) {
    double tlat1, tlon1;

    FindLatitudeLongitude(DrawInfo.Latitude,
                          DrawInfo.Longitude,
                          DerivedDrawInfo.WindBearing,
                          DerivedDrawInfo.WindSpeed,
                          &tlat1, &tlon1);
    traildrift_lat = (DrawInfo.Latitude-tlat1);
    traildrift_lon = (DrawInfo.Longitude-tlon1);
  }

  // JMW don't draw first bit from home airport

  int ntrail;
  if (TrailActive!=2) {
    ntrail = TRAILSIZE;
  } else {
    ntrail = TRAILSIZE/TRAILSHRINK;
  }
  if ((DisplayMode == dmCircling)) {
    ntrail /= TRAILSHRINK;
  }

  float this_vmax = (float)(1.5*min(5.0, max(MACCREADY,0.5)));
  float this_vmin = (float)(-1.5*min(5.0, max(MACCREADY,2.0)));
  vmax = this_vmax;
  vmin = this_vmin;

  const int skipdivisor = ntrail/5;
  int skipborder = skipdivisor;
  int skiplevel= 3; // JMW TODO, try lower level?
  POINT lastdrawn;

  lastdrawn.x = 0;
  lastdrawn.y = 0;
  int kd = TRAILSIZE+iSnailNext-ntrail;
  while (kd>= TRAILSIZE) {
    kd -= TRAILSIZE;
  }
  while (kd< 0) {
    kd += TRAILSIZE;
  }
  const int zerooffset = (TRAILSIZE-kd);
  skipborder += zerooffset % skiplevel;
  // TODO: Divide by time step cruise/circling for zerooffset

  bool thisvisible = true;
  bool lastvisible = false;
  float vclose = 0;
  int nclose = 0;
  int is = ((int)DrawInfo.Time)%skiplevel;
  const bool display_circling = DisplayMode == dmCircling;
  const double dtime = DrawInfo.Time;
  const rectObj bounds = screenbounds_latlon;

  const int deg = DEG_TO_INT(AngleLimit360(DisplayAngle));
  const int cost = ICOSTABLE[deg];
  const int sint = ISINETABLE[deg];
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const double mDrawScale = DrawScale;
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;

  for(i=1;i< ntrail; ++i)
  {
    if (i>=skipborder) {
      skiplevel= max(1,skiplevel-1);
      skipborder= i+2*(zerooffset % skiplevel)+skipdivisor;
      is = skiplevel;
    }

    is++;
    if ((i<ntrail-1) && (is < skiplevel)) {
      continue;
    } else {
      is=0;
    }

    j= kd+i;
    while (j>=TRAILSIZE) {
      j-= TRAILSIZE;
    }

    P1 = SnailTrail[j];

    if (((TrailFirstTime<0) || (P1.Time<TrailFirstTime)) && (P1.Time>=0)) {
      TrailFirstTime = P1.Time;
    }

    if (display_circling) {
      if ((!P1.Circling)&&( i<ntrail-60 )) {
        // ignore cruise mode lines unless very recent

	first = true;
	lastvisible = false;
        continue;
      }
    } else {
      //  if ((P1.Circling)&&( j%5 != 0 )) {
        // JMW TODO: This won't work properly!
        // draw only every 5 points from circling when in cruise mode
	//        continue;
      //      }
    }

    if (!P1.FarVisible) {
      first = true;
      lastvisible = false;
      continue;
    }

    thisvisible =   ((P1.Longitude> bounds.minx) &&
		     (P1.Longitude< bounds.maxx) &&
		     (P1.Latitude> bounds.miny) &&
		     (P1.Latitude< bounds.maxy)) ;

    // now we know both points are visible, better get screen coords
    // if we don't already.

    double this_lon, this_lat;
    if (dotraildrift) {
      double dt;
      dt = max(0,(dtime-P1.Time)*P1.DriftFactor);
      this_lon = P1.Longitude+traildrift_lon*dt;
      this_lat = P1.Latitude+traildrift_lat*dt;
    } else {
      this_lon = P1.Longitude;
      this_lat = P1.Latitude;
    }
#if 1
    // this is faster since many parameters are const
    int Y = Real2Int((mPanLatitude-this_lat)*mDrawScale);
    int X = Real2Int((mPanLongitude-this_lon)*fastcosine(this_lat)*mDrawScale);
    P1.Screen.x = (xxs-X*cost + Y*sint)/1024;
    P1.Screen.y = (Y*cost + X*sint + yys)/1024;
#else
    LatLon2Screen(this_lon,
		  this_lat,
		  P1.Screen);
#endif

    if (lastvisible && thisvisible) {
      if (abs(P1.Screen.y-lastdrawn.y)
	  +abs(P1.Screen.x-lastdrawn.x)<IBLSCALE(4)) {
	vclose += P1.Vario;
	nclose ++;
	continue;
	// don't draw if very short line
      }
    }

    // ok, we got this far, so draw the line
    // get the colour if it doesn't exist

    if (thisvisible || lastvisible) {
      if ((P1.Colour<0)||(P1.Colour>=NUMSNAILCOLORS)) {
	float cv = P1.Vario;
	if (nclose) {
	  // set color to average if skipped
	  cv = (cv+vclose)/(nclose+1);
	  nclose= 0;
	  vclose= 0;
	}
	if (cv<0) {
	  cv /= (-vmin); // JMW fixed bug here
	} else {
	  cv /= vmax;
	}
	P1.Colour = fSnailColour(cv);
      }
      SelectObject(hdc, hSnailPens[P1.Colour]);
    }

#ifdef NOLINETO
    if (lastvisible) {     // draw set cursor at P1
      if (dotraildrift) {
        double dt;
        dt = max(0,(dtime-P2.Time));
        LatLon2Screen(P2.Longitude+traildrift_lon*dt,
                      P2.Latitude+traildrift_lat*dt,
                      P2.Screen);
      } else {
        LatLon2Screen(P2.Longitude,
                      P2.Latitude,
                      P2.Screen);
      }
    }
    if (!first) {
      DrawSolidLine(hdc, P1.Screen, P2.Screen);
      lastdrawn = P1.Screen;
    } else {
      first = false;
    }
    P2 = P1;
#else
    if (!lastvisible) { // draw set cursor at P1
      MoveToEx(hdc, P1.Screen.x, P1.Screen.y, NULL);
    } else {
      LineTo(hdc, P1.Screen.x, P1.Screen.y);
      lastdrawn = P1.Screen;
    }
#endif
    lastvisible = thisvisible;
  }

  // draw final point to glider
#ifndef NOLINETO
  if (lastvisible) {
    LineTo(hdc, Orig.x, Orig.y);
  }
#endif
  return TrailFirstTime;
}


extern OLCOptimizer olc;

void MapWindow::DrawTrailFromTask(HDC hdc, const RECT rc,
				  const double TrailFirstTime) {
  static POINT ptin[MAXCLIPPOLYGON];

  if((TrailActive!=3) || (DisplayMode == dmCircling) || (TrailFirstTime<0))
    return;

  const double mTrailFirstTime = TrailFirstTime - DerivedDrawInfo.TakeOffTime;
  // since olc keeps track of time wrt takeoff

  olc.SetLine();
  int n = min(MAXCLIPPOLYGON,olc.getN());
  int i, j=0;
  for (i=0; i<n; i++) {
    if (olc.getTime(i)>= mTrailFirstTime)
      break;
    LatLon2Screen(olc.getLongitude(i),
                  olc.getLatitude(i),
                  ptin[j]);
    j++;
  }
  if (j>=2) {
    SelectObject(hdc,hSnailPens[NUMSNAILCOLORS/2]);
    ClipPolygon(hdc, ptin, j, rc, false);
  }
}


///////
extern HFONT  TitleWindowFont;

void MapWindow::DrawOffTrackIndicator(HDC hdc) {
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


void MapWindow::CalculateScreenPositionsGroundline(void) {
  if (FinalGlideTerrain) {
    LatLon2Screen(DerivedDrawInfo.GlideFootPrint,
		  Groundline, NUMTERRAINSWEEPS+1, 1);
  }
}


void MapWindow::DrawTerrainAbove(HDC hDC, RECT rc) {

  if (!DerivedDrawInfo.Flying) return;

  COLORREF whitecolor = RGB(0xff,0xff,0xff);
  COLORREF graycolor = RGB(0xf0,0xf0,0xf0);
  COLORREF origcolor = SetTextColor(hDCTemp, whitecolor);

  SetBkMode(hDCTemp, TRANSPARENT);

  SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
  SetBkColor(hDCTemp, whitecolor);

  SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  SetTextColor(hDCTemp, graycolor);
  SelectObject(hDCTemp, hAboveTerrainBrush); // hAirspaceBrushes[3] or 6
  Rectangle(hDCTemp,rc.left,rc.top,rc.right,rc.bottom);

  SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
  SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
  Polygon(hDCTemp,Groundline,NUMTERRAINSWEEPS+1);

  // need to do this to prevent drawing of colored outline
  SelectObject(hDCTemp, GetStockObject(WHITE_PEN));
#if (WINDOWSPC<1)
    TransparentImage(hDC,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     hDCTemp,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     whitecolor
                     );

#else
    TransparentBlt(hDC,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   hDCTemp,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );
  #endif

  // restore original color
  SetTextColor(hDCTemp, origcolor);
  SetBkMode(hDCTemp,OPAQUE);

}


void MapWindow::DrawProjectedTrack(HDC hdc, POINT Orig) {
  if ((ActiveWayPoint<=0) || !ValidTaskPoint(ActiveWayPoint) || !AATEnabled) {
    return;
  }
  if (DerivedDrawInfo.Circling || TaskIsTemporary()) {
    // don't display in various modes
    return;
  }

  // TODO: maybe have this work even if no task?
  // TODO: draw this also when in target pan mode

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
  DrawDashLine(hdc, 2, pt[0], pt[1], RGB(0,0,0));
}
