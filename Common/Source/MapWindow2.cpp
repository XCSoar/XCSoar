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
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>

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

  $Id$
}
*/

#include "StdAfx.h"
#include "options.h"
#include "Math/Screen.hpp"
#include "MapWindow.h"
#include "OnLineContest.h"
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
#include "VarioSound.h"
#include "InputEvents.h"
// #include <assert.h>
#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "Task.h"

#include "Terrain.h"
#include "RasterTerrain.h"

#include "GaugeVarioAltA.h"
#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"
#include "Screen/Util.hpp"

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

extern HFONT MapLabelFont;
extern HFONT  MapWindowBoldFont;


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



double MapWindow::findMapScaleBarSize(const RECT rc) {

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


void MapWindow::DrawMapScale2(HDC hDC, const RECT rc,
			      const POINT Orig_Aircraft)
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
      DrawSolidLine(hDC,Start,End, rc);
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
      DrawSolidLine(hDC,Start,End, rc);
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

      _Polyline(hDC, chevron, 3, rc);
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
  _Polyline(hDC, chevron, 2, rc);

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}
*/

#define fSnailColour(cv) max(0,min((short)(NUMSNAILCOLORS-1), (short)((cv+1.0)/2.0*NUMSNAILCOLORS)))

void MapWindow::DrawFLARMTraffic(HDC hDC, RECT rc, POINT Orig_Aircraft) {

  if (!EnableFLARMMap) return;

  if (!DrawInfo.FLARM_Available) return;

  HPEN hpOld;
  HPEN thinBlackPen = CreatePen(PS_SOLID, IBLSCALE(1), RGB(0,0,0));
  POINT Arrow[5];

  hpOld = (HPEN)SelectObject(hDC, thinBlackPen);

  int i;
//  double dX, dY;
  TextInBoxMode_t displaymode;
  displaymode.AsInt = 0;

  double screenrange = GetApproxScreenRange();
  double scalefact = screenrange/6000.0;

  HBRUSH redBrush = CreateSolidBrush(RGB(0xFF,0x00,0x00));
  HBRUSH yellowBrush = CreateSolidBrush(RGB(0xFF,0xFF,0x00));
  HBRUSH greenBrush = CreateSolidBrush(RGB(0x00,0xFF,0x00));

  for (i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (DrawInfo.FLARM_Traffic[i].ID!=0) {

      double target_lon;
      double target_lat;

      target_lon = DrawInfo.FLARM_Traffic[i].Longitude;
      target_lat = DrawInfo.FLARM_Traffic[i].Latitude;

      if ((EnableFLARMMap==2)&&(scalefact>1.0)) {
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

      // TODO feature: draw direction, rel height?
      POINT sc, sc_name, sc_av;
      LatLon2Screen(target_lon,
                    target_lat,
                    sc);

      sc_name = sc;

      sc_name.y -= IBLSCALE(16);
      sc_av = sc_name;

#ifndef FLARM_AVERAGE
      if (DrawInfo.FLARM_Traffic[i].Name) {
        TextInBox(hDC, DrawInfo.FLARM_Traffic[i].Name, sc.x+IBLSCALE(3),
                  sc.y, 0, displaymode,
                  true);
      }
#else
      TCHAR label_name[100];
      TCHAR label_avg[100];

      sc_av.x += IBLSCALE(3);

      if (DrawInfo.FLARM_Traffic[i].Name) {
	sc_name.y -= IBLSCALE(8);
	_stprintf(label_name, TEXT("%s"), DrawInfo.FLARM_Traffic[i].Name);
      } else {
	label_name[0]= _T('\0');
      }

      if (DrawInfo.FLARM_Traffic[i].Average30s>=0.1) {
	_stprintf(label_avg, TEXT("%.1f"),
		  LIFTMODIFY*DrawInfo.FLARM_Traffic[i].Average30s);
      } else {
	label_avg[0]= _T('\0');
      }

#ifndef NDEBUG
      // for testing only!
      _stprintf(label_avg, TEXT("2.3"));
      _stprintf(label_name, TEXT("WUE"));
#endif

      float vmax = (float)(1.5*min(5.0, max(MACCREADY,0.5)));
      float vmin = (float)(-1.5*min(5.0, max(MACCREADY,2.0)));

      float cv = DrawInfo.FLARM_Traffic[i].Average30s;
      if (cv<0) {
        cv /= (-vmin); // JMW fixed bug here
      } else {
        cv /= vmax;
      }

      int colourIndex = fSnailColour(cv);

      // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

      int dx = (sc_av.x-Orig_Aircraft.x);
      int dy = (sc_av.y-Orig_Aircraft.y);

      if (dx*dx+dy*dy > IBLSCALE(30)*IBLSCALE(30)) {
	// only draw labels if not close to aircraft

	HGDIOBJ oldFont = SelectObject(hDC, MapLabelFont);
	COLORREF oldTextColor = SetTextColor(hDC, RGB(0,0,0));

	if (_tcslen(label_name)>0) {
	  ExtTextOut(hDC, sc_name.x, sc_name.y, ETO_OPAQUE, NULL, label_name, _tcslen(label_name), NULL);
	}

	if (_tcslen(label_avg)>0) {
	  int size = _tcslen(label_avg);
	  SIZE tsize;
	  RECT brect;

	  GetTextExtentPoint(hDC, label_avg, size, &tsize);
	  brect.left = sc_av.x-2;
	  brect.right = brect.left+tsize.cx+6;
	  brect.top = sc_av.y+((tsize.cy+4)>>3)-2;
	  brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

	  HPEN hpOld = (HPEN)SelectObject(hDC, hSnailPens[colourIndex]);
	  HBRUSH hVarioBrush = CreateSolidBrush(hSnailColours[colourIndex]);
	  HBRUSH hbOld = (HBRUSH)SelectObject(hDC, hVarioBrush);

	  RoundRect(hDC, brect.left, brect.top, brect.right, brect.bottom,
		    IBLSCALE(8), IBLSCALE(8));

#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, sc_av.x, sc_av.y, 0, NULL, label_avg, size, NULL);
#else
      ExtTextOut(hDC, sc_av.x, sc_av.y, ETO_OPAQUE, NULL, label_avg, size, NULL);
#endif
	  SelectObject(hDC, hpOld);
	  SelectObject(hDC, hbOld);
	  DeleteObject(hVarioBrush);

	}

	SelectObject(hDC, oldFont);
	SetTextColor(hDC, oldTextColor);

      }

#endif
      if ((DrawInfo.FLARM_Traffic[i].AlarmLevel>0)
	  && (DrawInfo.FLARM_Traffic[i].AlarmLevel<4)) {
	DrawBitmapIn(hDC, sc, hFLARMTraffic);
      }

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

      switch (DrawInfo.FLARM_Traffic[i].AlarmLevel) {
      case 1:
	  SelectObject(hDC, yellowBrush);
	  break;
      case 2:
      case 3:
	  SelectObject(hDC, redBrush);
	  break;
      case 0:
      case 4:
	  SelectObject(hDC, greenBrush);
	  break;
      }

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         DrawInfo.FLARM_Traffic[i].TrackBearing - DisplayAngle);
      Polygon(hDC,Arrow,5);

    }
  }

  SelectObject(hDC, hpOld);

  DeleteObject((HPEN)thinBlackPen);
  DeleteObject(greenBrush);
  DeleteObject(yellowBrush);
  DeleteObject(redBrush);

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
  if (iAirspaceMode[circ.Type]%2 == 1) {
    double basealt;
    double topalt;
    if (circ.Base.Base != abAGL) {
      basealt = circ.Base.Altitude;
    } else {
      basealt = circ.Base.AGL + CALCULATED_INFO.TerrainAlt;
    }
    if (circ.Top.Base != abAGL) {
      topalt = circ.Top.Altitude;
    } else {
      topalt = circ.Top.AGL + CALCULATED_INFO.TerrainAlt;
    }
    if(CheckAirspaceAltitude(basealt, topalt)) {
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
}

void MapWindow::CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA &area) {
  area.Visible = false;
  if (!area.FarVisible) return;
  if (iAirspaceMode[area.Type]%2 == 1) {
    double basealt;
    double topalt;
    if (area.Base.Base != abAGL) {
      basealt = area.Base.Altitude;
    } else {
      basealt = area.Base.AGL + CALCULATED_INFO.TerrainAlt;
    }
    if (area.Top.Base != abAGL) {
      topalt = area.Top.Altitude;
    } else {
      topalt = area.Top.AGL + CALCULATED_INFO.TerrainAlt;
    }
    if(CheckAirspaceAltitude(basealt, topalt)) {
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


void MapWindow::DrawHorizon(HDC hDC, const RECT rc)
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
            a1, a2, RGB(0,0,0), rc);

  a1.x = Start.x;
  a1.y = Start.y-radius/4;
  _DrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, Start, RGB(0,0,0), rc);

  //

#define ROOT2 0.70711

  int rr2p = lround(radius*ROOT2+IBLSCALE(1));
  int rr2n = lround(radius*ROOT2);

  a1.x = Start.x+rr2p;
  a1.y = Start.y-rr2p;
  a2.x = Start.x+rr2n;
  a2.y = Start.y-rr2n;

  _DrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0), rc);

  a1.x = Start.x-rr2p;
  a1.y = Start.y-rr2p;
  a2.x = Start.x-rr2n;
  a2.y = Start.y-rr2n;

  _DrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0), rc);

  // JMW experimental, display stall sensor
  double s = max(0.0,min(1.0,DrawInfo.StallRatio));
  long m = (long)((rc.bottom-rc.top)*s*s);
  a1.x = rc.right-1;
  a1.y = rc.bottom-m;
  a2.x = a1.x-10;
  a2.y = a1.y;
  _DrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, a2, RGB(0xff,0,0), rc);

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


void MapWindow::DrawGreatCircle(HDC hdc,
                                double startLon, double startLat,
                                double targetLon, double targetLat,
				const RECT rc) {


#if OLD_GREAT_CIRCLE
  // TODO accuracy: this is actually wrong, it should recalculate the
  // bearing each step
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
    DrawSolidLine(hdc, StartP, EndP, rc);
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

      DrawSolidLine(hdc, StartP, EndP, rc);

      StartP.x = EndP.x;
      StartP.y = EndP.y;

      startLat = tlat1;
      startLon = tlon1;

    }
  }
#else
  // Simple and this should work for PNA with display bug

  HPEN hpOld = (HPEN)SelectObject(hdc, hpBearing);
  POINT pt[2];
  LatLon2Screen(startLon,
                startLat,
                pt[0]);
  LatLon2Screen(targetLon,
                targetLat,
                pt[1]);
  ClipPolygon(hdc, pt, 2, rc, false);

#endif
  SelectObject(hdc, hpOld);
}



int MapWindow::iSnailNext=0;

// This function is slow...
double MapWindow::DrawTrail( HDC hdc, const POINT Orig, const RECT rc)
{
  int i, snail_index;
  SNAIL_POINT P1;
  static BOOL last_circling = FALSE;
  static float vario_max= 5.0;
  static float vario_min= -5.0;
  static bool need_colour = true;

  double TrailFirstTime = -1;

  if(!TrailActive)
    return -1;

  if ((DisplayMode == dmCircling) != last_circling) {
    need_colour = true;
  }
  last_circling = (DisplayMode == dmCircling);

  //////////// Trail drift calculations

  double traildrift_lat = 0.0;
  double traildrift_lon = 0.0;

  if (EnableTrailDrift && (DisplayMode == dmCircling)) {
    double tlat1, tlon1;

    FindLatitudeLongitude(DrawInfo.Latitude,
                          DrawInfo.Longitude,
                          DerivedDrawInfo.WindBearing,
                          DerivedDrawInfo.WindSpeed,
                          &tlat1, &tlon1);
    traildrift_lat = (DrawInfo.Latitude-tlat1);
    traildrift_lon = (DrawInfo.Longitude-tlon1);
  } else {
    traildrift_lat = 0.0;
    traildrift_lon = 0.0;
  }

  // JMW don't draw first bit from home airport

  /////////////  Trail size

  int num_trail_max;
  if (TrailActive!=2) {
    num_trail_max = TRAILSIZE;
  } else {
    num_trail_max = TRAILSIZE/TRAILSHRINK;
  }
  if ((DisplayMode == dmCircling)) {
    num_trail_max /= TRAILSHRINK;
  }

  ///////////// Vario colour scaling

  float this_vario_max = (float)(1.5*min(5.0, max(MACCREADY,0.5)));
  float this_vario_min = (float)(-1.5*min(5.0, max(MACCREADY,2.0)));
  vario_max = this_vario_max;
  vario_min = this_vario_min;

  ///////////// Snail skipping

  const int skip_divisor = num_trail_max/5;
  int skip_border = skip_divisor;
  int skip_level= 3; // TODO code: try lower level?

  int snail_offset = TRAILSIZE+iSnailNext-num_trail_max;
  while (snail_offset>= TRAILSIZE) {
    snail_offset -= TRAILSIZE;
  }
  while (snail_offset< 0) {
    snail_offset += TRAILSIZE;
  }
  const int zero_offset = (TRAILSIZE-snail_offset);
  skip_border += zero_offset % skip_level;

  int index_skip = ((int)DrawInfo.Time)%skip_level;

  // TODO code: Divide by time step cruise/circling for zero_offset

  ///////////// Keep track of what's drawn

  bool this_visible = true;
  bool last_visible = false;
  POINT point_lastdrawn;
  point_lastdrawn.x = 0;
  point_lastdrawn.y = 0;

  ///////////// Average colour display for skipped points
  float vario_av = 0;
  int vario_av_num = 0;

  ///////////// Constants for speedups

  const bool display_circling = DisplayMode == dmCircling;
  const double display_time = DrawInfo.Time;

  // expand bounds so in strong winds the appropriate snail points are
  // still visible (since they are being tested before drift is applied)
  // this expands them by one minute

  rectObj bounds_thermal = screenbounds_latlon;
  screenbounds_latlon.minx -= fabs(60.0*traildrift_lon);
  screenbounds_latlon.maxx += fabs(60.0*traildrift_lon);
  screenbounds_latlon.miny -= fabs(60.0*traildrift_lat);
  screenbounds_latlon.maxy += fabs(60.0*traildrift_lat);

  const rectObj bounds = bounds_thermal;

  const int deg = DEG_TO_INT(AngleLimit360(DisplayAngle));
  const int cost = ICOSTABLE[deg];
  const int sint = ISINETABLE[deg];
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const double mDrawScale = DrawScale;
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;

  ////////////// Main loop

  for(i=1;i< num_trail_max; ++i)
  {
    ///// Handle skipping

    if (i>=skip_border) {
      skip_level= max(1,skip_level-1);
      skip_border= i+2*(zero_offset % skip_level)+skip_divisor;
      index_skip = skip_level;
    }

    index_skip++;
    if ((i<num_trail_max-10) && (index_skip < skip_level)) {
      continue;
    } else {
      index_skip=0;
    }

    ////// Find the snail point

    snail_index = snail_offset+i;
    while (snail_index>=TRAILSIZE) {
      snail_index-= TRAILSIZE;
    }

    P1 = SnailTrail[snail_index];

    /////// Mark first time of display point

    if (((TrailFirstTime<0) || (P1.Time<TrailFirstTime)) && (P1.Time>=0)) {
      TrailFirstTime = P1.Time;
    }

    //////// Ignoring display elements for modes

    if (display_circling) {
      if ((!P1.Circling)&&( i<num_trail_max-60 )) {
        // ignore cruise mode lines unless very recent
	last_visible = false;
        continue;
      }
    } else {
      //  if ((P1.Circling)&&( snail_index % 5 != 0 )) {
        // JMW TODO code: This won't work properly!
        // draw only every 5 points from circling when in cruise mode
	//        continue;
      //      }
    }

    ///////// Filter if far visible

    if (!P1.FarVisible) {
      last_visible = false;
      continue;
    }

    ///////// Determine if this is visible

    this_visible =   ((P1.Longitude> bounds.minx) &&
		     (P1.Longitude< bounds.maxx) &&
		     (P1.Latitude> bounds.miny) &&
		     (P1.Latitude< bounds.maxy)) ;

    if (!this_visible && !last_visible) {
      last_visible = false;
      continue;
    }

    ////////// Find coordinates on screen after applying trail drift

    // now we know either point is visible, better get screen coords
    // if we don't already.

    double dt = max(0,(display_time-P1.Time)*P1.DriftFactor);
    double this_lon = P1.Longitude+traildrift_lon*dt;
    double this_lat = P1.Latitude+traildrift_lat*dt;

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

    ////////// Determine if we should skip if close to previous point

    if (last_visible && this_visible) {
      // only average what's visible

      if (abs(P1.Screen.y-point_lastdrawn.y)
	  +abs(P1.Screen.x-point_lastdrawn.x)<IBLSCALE(4)) {
	vario_av += P1.Vario;
	vario_av_num ++;
	continue;
	// don't draw if very short line
      }
    }

    ////////// Lookup the colour if it's not already set

    if ((P1.Colour<0)||(P1.Colour>=NUMSNAILCOLORS)) {
      float colour_vario = P1.Vario;
      if (vario_av_num) {
	// set color to average if skipped
	colour_vario = (colour_vario+vario_av)/(vario_av_num+1);
	vario_av_num= 0;
	vario_av= 0;
      }
      if (colour_vario<0) {
	colour_vario /= (-vario_min); // JMW fixed bug here
      } else {
	colour_vario /= vario_max;
      }
      P1.Colour = fSnailColour(colour_vario);
    }
    SelectObject(hdc, hSnailPens[P1.Colour]);

    if (!last_visible) { // draw set cursor at P1
#ifndef NOLINETO
      MoveToEx(hdc, P1.Screen.x, P1.Screen.y, NULL);
#endif
    } else {
#ifndef NOLINETO
      LineTo(hdc, P1.Screen.x, P1.Screen.y);
#else
      DrawSolidLine(hdc, P1.Screen, point_lastdrawn, rc);
#endif
    }
    point_lastdrawn = P1.Screen;
    last_visible = this_visible;
  }

  // draw final point to glider
  if (last_visible) {
#ifndef NOLINETO
    LineTo(hdc, Orig.x, Orig.y);
#else
    DrawSolidLine(hdc, Orig, point_lastdrawn, rc);
#endif
  }

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


void MapWindow::CalculateScreenPositionsGroundline(void) {
  if (FinalGlideTerrain) {
    LatLon2Screen(DerivedDrawInfo.GlideFootPrint,
		  Groundline, NUMTERRAINSWEEPS+1, 1);
  }
}


void MapWindow::DrawTerrainAbove(HDC hDC, const RECT rc, HDC buffer) {

  if (!DerivedDrawInfo.Flying) return;

  COLORREF whitecolor = RGB(0xff,0xff,0xff);
  COLORREF graycolor = RGB(0xf0,0xf0,0xf0);
  COLORREF origcolor = SetTextColor(buffer, whitecolor);

  SetBkMode(buffer, TRANSPARENT);

  SetBkColor(buffer, whitecolor);

  SelectObject(buffer, GetStockObject(WHITE_PEN));
  SetTextColor(buffer, graycolor);
  SelectObject(buffer, hAboveTerrainBrush); // hAirspaceBrushes[3] or 6
  Rectangle(buffer,rc.left,rc.top,rc.right,rc.bottom);

  SelectObject(buffer, GetStockObject(WHITE_PEN));
  SelectObject(buffer, GetStockObject(WHITE_BRUSH));
  Polygon(buffer,Groundline,NUMTERRAINSWEEPS+1);

  // need to do this to prevent drawing of colored outline
  SelectObject(buffer, GetStockObject(WHITE_PEN));
#if (WINDOWSPC<1)
    TransparentImage(hDC,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     buffer,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     whitecolor
                     );

#else
    TransparentBlt(hDC,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   buffer,
                   rc.left,rc.top,
                   rc.right-rc.left,rc.bottom-rc.top,
                   whitecolor
                   );
  #endif

  // restore original color
  SetTextColor(buffer, origcolor);
  SetBkMode(buffer, OPAQUE);

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





void MapWindow::DrawTeammate(HDC hdc, RECT rc)
{
  POINT point;

  if (TeammateCodeValid)
    {
      if(PointVisible(TeammateLongitude, TeammateLatitude) )
	{
	  LatLon2Screen(TeammateLongitude, TeammateLatitude, point);

	  SelectObject(hDCTemp,hBmpTeammatePosition);
	  DrawBitmapX(hdc,
		      point.x-IBLSCALE(10),
		      point.y-IBLSCALE(10),
		      20,20,
		      hDCTemp,0,0,SRCPAINT);

	  DrawBitmapX(hdc,
		      point.x-IBLSCALE(10),
		      point.y-IBLSCALE(10),
		      20,20,
		      hDCTemp,20,0,SRCAND);
	}
    }
}



void MapWindow::DrawThermalBand(HDC hDC, const RECT rc)
{
  POINT GliderBand[5] = { {0,0},{23,0},{22,0},{24,0},{0,0} };

  if ((DerivedDrawInfo.TaskAltitudeDifference>50)
      &&(DisplayMode == dmFinalGlide)) {
    return;
  }

  // JMW TODO accuracy: gather proper statistics
  // note these should/may also be relative to ground
  int i;
  double mth = DerivedDrawInfo.MaxThermalHeight;
  double maxh, minh;
  double h;
  double Wt[NUMTHERMALBUCKETS];
  double ht[NUMTHERMALBUCKETS];
  double Wmax=0.0;
  int TBSCALEY = ( (rc.bottom - rc.top )/2)-IBLSCALE(30);
#define TBSCALEX 20

  // calculate height above safety altitude
  double hoffset = SAFETYALTITUDEBREAKOFF+DerivedDrawInfo.TerrainBase;
  h = DerivedDrawInfo.NavAltitude-hoffset;

  bool draw_start_height = ((ActiveWayPoint==0) && (ValidTaskPoint(0))
			    && (StartMaxHeight!=0)
			    && (DerivedDrawInfo.TerrainValid));
  double hstart=0;
  if (draw_start_height) {
    if (StartHeightRef == 0) {
      hstart = StartMaxHeight+DerivedDrawInfo.TerrainAlt;
    } else {
      hstart = StartMaxHeight;
    }
    hstart -= hoffset;
  }

  // calculate top/bottom height
  maxh = max(h, mth);
  minh = min(h, 0);

  if (draw_start_height) {
    maxh = max(maxh, hstart);
    minh = min(minh, hstart);
  }

  // no thermalling has been done above safety altitude
  if (mth<=1) {
    return;
  }
  if (maxh-minh<=0) {
    return;
  }

  // normalised heights
  double hglider = (h-minh)/(maxh-minh);
  hstart = (hstart-minh)/(maxh-minh);

  // calculate averages
  int numtherm = 0;

  double mc = MACCREADY;
  Wmax = max(0.5,mc);

  for (i=0; i<NUMTHERMALBUCKETS; i++) {
    double wthis = 0;
    // height of this thermal point [0,mth]
    double hi = i*mth/NUMTHERMALBUCKETS;
    double hp = ((hi-minh)/(maxh-minh));

    if (DerivedDrawInfo.ThermalProfileN[i]>5) {
      // now requires 10 items in bucket before displaying,
      // to eliminate kinks
      wthis = DerivedDrawInfo.ThermalProfileW[i]
                 /DerivedDrawInfo.ThermalProfileN[i];
    }
    if (wthis>0.0) {
      ht[numtherm]= hp;
      Wt[numtherm]= wthis;
      Wmax = max(Wmax,wthis/1.5);
      numtherm++;
    }
  }

  if ((!draw_start_height) && (numtherm<=1)) {
    return; // don't display if insufficient statistics
    // but do draw if start height needs to be drawn
  }

  // drawing info
  HPEN hpOld;

  // position of thermal band
  if (numtherm>1) {
    hpOld = (HPEN)SelectObject(hDC, hpThermalBand);
    HBRUSH hbOld = (HBRUSH)SelectObject(hDC, hbThermalBand);

    POINT ThermalProfile[NUMTHERMALBUCKETS+2];
    for (i=0; i<numtherm; i++) {
      ThermalProfile[1+i].x =
	(iround((Wt[i]/Wmax)*IBLSCALE(TBSCALEX)))+rc.left;

      ThermalProfile[1+i].y =
	IBLSCALE(4)+iround(TBSCALEY*(1.0-ht[i]))+rc.top;
    }
    ThermalProfile[0].x = rc.left;
    ThermalProfile[0].y = ThermalProfile[1].y;
    ThermalProfile[numtherm+1].x = rc.left;
    ThermalProfile[numtherm+1].y = ThermalProfile[numtherm].y;

    Polygon(hDC,ThermalProfile,numtherm+2);
    SelectObject(hDC, hbOld);
  }

  // position of thermal band

  GliderBand[0].y = IBLSCALE(4)+iround(TBSCALEY*(1.0-hglider))+rc.top;
  GliderBand[1].y = GliderBand[0].y;
  GliderBand[1].x = max(iround((mc/Wmax)*IBLSCALE(TBSCALEX)),IBLSCALE(4))
    +rc.left;

  GliderBand[2].x = GliderBand[1].x-IBLSCALE(4);
  GliderBand[2].y = GliderBand[0].y-IBLSCALE(4);
  GliderBand[3].x = GliderBand[1].x;
  GliderBand[3].y = GliderBand[1].y;
  GliderBand[4].x = GliderBand[1].x-IBLSCALE(4);
  GliderBand[4].y = GliderBand[0].y+IBLSCALE(4);

  hpOld = (HPEN)SelectObject(hDC, hpThermalBandGlider);

  Polyline(hDC,GliderBand, 2);
  Polyline(hDC,GliderBand+2, 3); // arrow head

  if (draw_start_height) {
    SelectObject(hDC, hpFinalGlideBelow);
    GliderBand[0].y = IBLSCALE(4)+iround(TBSCALEY*(1.0-hstart))+rc.top;
    GliderBand[1].y = GliderBand[0].y;
    Polyline(hDC, GliderBand, 2);
  }

  SelectObject(hDC, hpOld);

}


void MapWindow::DrawFinalGlide(HDC hDC, const RECT rc)
{

  /*
  POINT Scale[18] = {
    {5,-50 }, {14,-60 }, {23, -50},
    {5,-40 }, {14,-50 }, {23, -40},
    {5,-30 }, {14,-40 }, {23, -30},
    {5,-20 }, {14,-30 }, {23, -20},
    {5,-10 }, {14,-20 }, {23, -10},
    {5, 0  }, {14,-10 }, {23,   0},
  };*/

  POINT GlideBar[6] =
    { {0,0},{9,-9},{18,0},{18,0},{9,0},{0,0} };
  POINT GlideBar0[6] =
    { {0,0},{9,-9},{18,0},{18,0},{9,0},{0,0} };

  HPEN hpOld;
  HBRUSH hbOld;

  TCHAR Value[10];

  int Offset;
  int Offset0;
  int i;

  LockTaskData();  // protect from external task changes
  #ifdef HAVEEXCEPTIONS
  __try{
  #endif

    if (ValidTaskPoint(ActiveWayPoint)){
    // if (ActiveWayPoint >= 0) {

      const int y0 = ( (rc.bottom - rc.top )/2)+rc.top;

      // 60 units is size, div by 8 means 60*8 = 480 meters.

      Offset = ((int)DerivedDrawInfo.TaskAltitudeDifference)/8;
      Offset0 = ((int)DerivedDrawInfo.TaskAltitudeDifference0)/8;
      // TODO feature: should be an angle if in final glide mode

      if(Offset > 60) Offset = 60;
      if(Offset < -60) Offset = -60;
      Offset = IBLSCALE(Offset);
      if(Offset<0) {
        GlideBar[1].y = IBLSCALE(9);
      }

      if(Offset0 > 60) Offset0 = 60;
      if(Offset0 < -60) Offset0 = -60;
      Offset0 = IBLSCALE(Offset0);
      if(Offset0<0) {
        GlideBar0[1].y = IBLSCALE(9);
      }

      for(i=0;i<6;i++)
        {
          GlideBar[i].y += y0;
          GlideBar[i].x = IBLSCALE(GlideBar[i].x)+rc.left;
        }
      GlideBar[0].y -= Offset;
      GlideBar[1].y -= Offset;
      GlideBar[2].y -= Offset;

      for(i=0;i<6;i++)
        {
          GlideBar0[i].y += y0;
          GlideBar0[i].x = IBLSCALE(GlideBar0[i].x)+rc.left;
        }
      GlideBar0[0].y -= Offset0;
      GlideBar0[1].y -= Offset0;
      GlideBar0[2].y -= Offset0;

      if ((Offset<0)&&(Offset0<0)) {
        // both below
        if (Offset0!= Offset) {
          int dy = (GlideBar0[0].y-GlideBar[0].y)
            +(GlideBar0[0].y-GlideBar0[3].y);
          dy = max(IBLSCALE(3), dy);
          GlideBar[3].y = GlideBar0[0].y-dy;
          GlideBar[4].y = GlideBar0[1].y-dy;
          GlideBar[5].y = GlideBar0[2].y-dy;

          GlideBar0[0].y = GlideBar[3].y;
          GlideBar0[1].y = GlideBar[4].y;
          GlideBar0[2].y = GlideBar[5].y;
        } else {
          Offset0 = 0;
        }

      } else if ((Offset>0)&&(Offset0>0)) {
        // both above
        GlideBar0[3].y = GlideBar[0].y;
        GlideBar0[4].y = GlideBar[1].y;
        GlideBar0[5].y = GlideBar[2].y;

        if (abs(Offset0-Offset)<IBLSCALE(4)) {
          Offset= Offset0;
        }
      }

      // draw actual glide bar
      if (Offset<=0) {
        if (LandableReachable) {
          hpOld = (HPEN)SelectObject(hDC, hpFinalGlideBelowLandable);
          hbOld = (HBRUSH)SelectObject(hDC, hbFinalGlideBelowLandable);
        } else {
          hpOld = (HPEN)SelectObject(hDC, hpFinalGlideBelow);
          hbOld = (HBRUSH)SelectObject(hDC, hbFinalGlideBelow);
        }
      } else {
        hpOld = (HPEN)SelectObject(hDC, hpFinalGlideAbove);
        hbOld = (HBRUSH)SelectObject(hDC, hbFinalGlideAbove);
      }
      Polygon(hDC,GlideBar,6);

      // draw glide bar at mc 0
      if (Offset0<=0) {
        if (LandableReachable) {
          SelectObject(hDC, hpFinalGlideBelowLandable);
          SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
        } else {
          SelectObject(hDC, hpFinalGlideBelow);
          SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
        }
      } else {
        SelectObject(hDC, hpFinalGlideAbove);
        SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
      }
      if (Offset!=Offset0) {
        Polygon(hDC,GlideBar0,6);
      }

      // JMW draw x on final glide bar if unreachable at current Mc
      // hpAircraftBorder
      if ((DerivedDrawInfo.TaskTimeToGo>0.9*ERROR_TIME)
	  || ((MACCREADY<0.01) && (DerivedDrawInfo.TaskAltitudeDifference<0))) {
	SelectObject(hDC, hpAircraftBorder);
	POINT Cross[4] = { {-5, -5},
			   { 5,  5},
			   {-5,  5},
			   { 5, -5} };
	for (i=0; i<4; i++) {
	  Cross[i].x = IBLSCALE(Cross[i].x+9);
	  Cross[i].y = IBLSCALE(Cross[i].y+9)+y0;
	}
        Polygon(hDC,Cross,2);
        Polygon(hDC,&Cross[2],2);
      }

      if (Appearance.IndFinalGlide == fgFinalGlideDefault){

        _stprintf(Value,TEXT("%1.0f "),
                  ALTITUDEMODIFY*DerivedDrawInfo.TaskAltitudeDifference);

        if (Offset>=0) {
          Offset = GlideBar[2].y+Offset+IBLSCALE(5);
        } else {
          if (Offset0>0) {
            Offset = GlideBar0[1].y-IBLSCALE(15);
          } else {
            Offset = GlideBar[2].y+Offset-IBLSCALE(15);
          }
        }

        TextInBoxMode_t TextInBoxMode = {1|8};
        TextInBox(hDC, Value, 0, (int)Offset, 0, TextInBoxMode);

      } else
        if (Appearance.IndFinalGlide == fgFinalGlideAltA){

          SIZE  TextSize;
          HFONT oldFont;
          int y = GlideBar[3].y;
          // was ((rc.bottom - rc.top )/2)-rc.top-
          //            Appearance.MapWindowBoldFont.CapitalHeight/2-1;
          int x = GlideBar[2].x+IBLSCALE(1);
          HBITMAP Bmp;
          POINT  BmpPos;
          POINT  BmpSize;

          _stprintf(Value, TEXT("%1.0f"),
                    Units::ToUserAltitude(DerivedDrawInfo.TaskAltitudeDifference));

          oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
          GetTextExtentPoint(hDC, Value, _tcslen(Value), &TextSize);

          SelectObject(hDC, GetStockObject(WHITE_BRUSH));
          SelectObject(hDC, GetStockObject(WHITE_PEN));
          Rectangle(hDC, x, y,
                    x+IBLSCALE(1)+TextSize.cx,
                    y+Appearance.MapWindowBoldFont.CapitalHeight+IBLSCALE(2));

          ExtTextOut(hDC, x+IBLSCALE(1),
                     y+Appearance.MapWindowBoldFont.CapitalHeight
                     -Appearance.MapWindowBoldFont.AscentHeight+IBLSCALE(1),
                     0, NULL, Value, _tcslen(Value), NULL);

          if (Units::GetUnitBitmap(Units::GetUserAltitudeUnit(), &Bmp, &BmpPos, &BmpSize, 0)){
            HBITMAP oldBitMap = (HBITMAP)SelectObject(hDCTemp, Bmp);
            DrawBitmapX(hDC, x+TextSize.cx+IBLSCALE(1), y, BmpSize.x, BmpSize.y,
                        hDCTemp, BmpPos.x, BmpPos.y, SRCCOPY);
            SelectObject(hDCTemp, oldBitMap);
          }

          SelectObject(hDC, oldFont);

        }

      SelectObject(hDC, hbOld);
      SelectObject(hDC, hpOld);
    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnlockTaskData();
     }

}


void MapWindow::DrawCompass(HDC hDC, const RECT rc)
{
  POINT Start;
  HPEN hpOld;
  HBRUSH hbOld;

  if (Appearance.CompassAppearance == apCompassDefault){

    Start.y = IBLSCALE(19)+rc.top;
    Start.x = rc.right - IBLSCALE(19);

    if (EnableVarioGauge && MapRectBig.right == rc.right)
        Start.x -= InfoBoxLayout::ControlWidth;

    POINT Arrow[5] = { {0,-18}, {-6,10}, {0,0}, {6,10}, {0,-18}};

    hpOld = (HPEN)SelectObject(hDC, hpCompass);
    hbOld = (HBRUSH)SelectObject(hDC, hbCompass);

    // North arrow
    PolygonRotateShift(Arrow, 5, Start.x, Start.y, -DisplayAngle);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hbOld);
    SelectObject(hDC, hpOld);

  } else
  if (Appearance.CompassAppearance == apCompassAltA){

    static double lastDisplayAngle = 9999.9;
    static int lastRcRight = 0;
    static POINT Arrow[5] = { {0,-11}, {-5,9}, {0,3}, {5,9}, {0,-11}};
    extern bool EnableVarioGauge;

    if (lastDisplayAngle != DisplayAngle || lastRcRight != rc.right){

      Arrow[0].x  = 0;
      Arrow[0].y  = -11;
      Arrow[1].x  = -5;
      Arrow[1].y  = 9;
      Arrow[2].x  = 0;
      Arrow[2].y  = 3;
      Arrow[3].x  = 5;
      Arrow[3].y  = 9;
      Arrow[4].x  = 0;
      Arrow[4].y  = -11;

      Start.y = rc.top + IBLSCALE(10);
      Start.x = rc.right - IBLSCALE(11);

      if (EnableVarioGauge && MapRectBig.right == rc.right) {
        Start.x -= InfoBoxLayout::ControlWidth;
      }

      // North arrow
      PolygonRotateShift(Arrow, 5, Start.x, Start.y,
                         -DisplayAngle);

      lastDisplayAngle = DisplayAngle;
      lastRcRight = rc.right;
    }

    hpOld = (HPEN)SelectObject(hDC, hpCompassBorder);
    hbOld = (HBRUSH)SelectObject(hDC, hbCompass);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hpCompass);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hbOld);
    SelectObject(hDC, hpOld);

  }

}

void MapWindow::ClearAirSpace(HDC dc, bool fill) {
  COLORREF whitecolor = RGB(0xff,0xff,0xff);

  SetTextColor(dc, whitecolor);
  SetBkMode(dc, TRANSPARENT);
  SetBkColor(dc, whitecolor);
  SelectObject(dc, GetStockObject(WHITE_PEN));
  SelectObject(dc, GetStockObject(WHITE_BRUSH));
  Rectangle(dc, MapRect.left, MapRect.top, MapRect.right, MapRect.bottom);
  if (fill) {
    SelectObject(dc, GetStockObject(WHITE_PEN));
  }
}

// TODO code: optimise airspace drawing
void MapWindow::DrawAirSpace(HDC hdc, const RECT rc, HDC buffer)
{
  COLORREF whitecolor = RGB(0xff,0xff,0xff);
  unsigned int i;

  bool found = false;

  if (AirspaceCircle) {
    // draw without border
    for(i=0;i<NumberOfAirspaceCircles;i++) {
      if (AirspaceCircle[i].Visible==2) {
	if (!found) {
          ClearAirSpace(buffer, true);
	  found = true;
	}
        // this color is used as the black bit
        SetTextColor(buffer,
                     Colours[iAirspaceColour[AirspaceCircle[i].Type]]);
        // get brush, can be solid or a 1bpp bitmap
        SelectObject(buffer,
                     hAirspaceBrushes[iAirspaceBrush[AirspaceCircle[i].Type]]);
        Circle(buffer,
               AirspaceCircle[i].Screen.x ,
               AirspaceCircle[i].Screen.y ,
               AirspaceCircle[i].ScreenR ,rc, true, true);
      }
    }
  }

  if (AirspaceArea) {
    for(i=0;i<NumberOfAirspaceAreas;i++) {
      if(AirspaceArea[i].Visible ==2) {
	if (!found) {
	  ClearAirSpace(buffer, true);
	  found = true;
	}
        // this color is used as the black bit
        SetTextColor(buffer,
                     Colours[iAirspaceColour[AirspaceArea[i].Type]]);
        SelectObject(buffer,
                     hAirspaceBrushes[iAirspaceBrush[AirspaceArea[i].Type]]);
        ClipPolygon(buffer,
                    AirspaceScreenPoint+AirspaceArea[i].FirstPoint,
                    AirspaceArea[i].NumPoints, rc, true);
      }
    }
  }

  ////////// draw it again, just the outlines

  if (found) {
    SelectObject(buffer, GetStockObject(HOLLOW_BRUSH));
    SelectObject(buffer, GetStockObject(WHITE_PEN));
  }

  if (AirspaceCircle) {
    for(i=0;i<NumberOfAirspaceCircles;i++) {
      if (AirspaceCircle[i].Visible) {
	if (!found) {
	  ClearAirSpace(buffer, false);
	  found = true;
	}
        if (bAirspaceBlackOutline) {
          SelectObject(buffer, GetStockObject(BLACK_PEN));
        } else {
          SelectObject(buffer, hAirspacePens[AirspaceCircle[i].Type]);
        }
        Circle(buffer,
               AirspaceCircle[i].Screen.x ,
               AirspaceCircle[i].Screen.y ,
               AirspaceCircle[i].ScreenR ,rc, true, false);
      }
    }
  }

  if (AirspaceArea) {
    for(i=0;i<NumberOfAirspaceAreas;i++) {
      if(AirspaceArea[i].Visible) {
	if (!found) {
	  ClearAirSpace(buffer, false);
	  found = true;
	}
        if (bAirspaceBlackOutline) {
          SelectObject(buffer, GetStockObject(BLACK_PEN));
        } else {
          SelectObject(buffer, hAirspacePens[AirspaceArea[i].Type]);
        }

	POINT *pstart = AirspaceScreenPoint+AirspaceArea[i].FirstPoint;
        ClipPolygon(buffer, pstart,
                    AirspaceArea[i].NumPoints, rc, false);

	if (AirspaceArea[i].NumPoints>2) {
	  // JMW close if open
	  if ((pstart[0].x != pstart[AirspaceArea[i].NumPoints-1].x) ||
	      (pstart[0].y != pstart[AirspaceArea[i].NumPoints-1].y)) {
	    POINT ps[2];
	    ps[0] = pstart[0];
	    ps[1] = pstart[AirspaceArea[i].NumPoints-1];
	    _Polyline(buffer, ps, 2, rc);
	  }
	}

      }
    }
  }

  if (found) {
    // need to do this to prevent drawing of colored outline
    SelectObject(buffer, GetStockObject(WHITE_PEN));
#if (WINDOWSPC<1)
    TransparentImage(hdc,
                     rc.left, rc.top,
                     rc.right-rc.left,rc.bottom-rc.top,
                     buffer,
                     rc.left, rc.top,
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
    // restore original color
    //    SetTextColor(hDCTemp, origcolor);
    SetBkMode(buffer, OPAQUE);
  }
}


void MapWindow::DrawMapScale(HDC hDC, const RECT rc /* the Map Rect*/,
                             const bool ScaleChangeFeedback)
{


  if (Appearance.MapScale == apMsDefault){

    TCHAR Scale[80];
    TCHAR TEMP[20];
    POINT Start, End;
    HPEN hpOld;
    hpOld = (HPEN)SelectObject(hDC, hpMapScale);

    Start.x = rc.right-IBLSCALE(6); End.x = rc.right-IBLSCALE(6);
    Start.y = rc.bottom-IBLSCALE(30); End.y = Start.y - IBLSCALE(30);
    DrawSolidLine(hDC,Start,End, rc);

    Start.x = rc.right-IBLSCALE(11); End.x = rc.right-IBLSCALE(6);
    End.y = Start.y;
    DrawSolidLine(hDC,Start,End, rc);

    Start.y = Start.y - IBLSCALE(30); End.y = Start.y;
    DrawSolidLine(hDC,Start,End, rc);

    SelectObject(hDC, hpOld);

    if(MapScale <0.1)
    {
      _stprintf(Scale,TEXT("%1.2f"),MapScale);
    }
    else if(MapScale <3)
    {
      _stprintf(Scale,TEXT("%1.1f"),MapScale);
    }
    else
    {
      _stprintf(Scale,TEXT("%1.0f"),MapScale);
    }

    _tcscat(Scale, Units::GetDistanceName());

    if (AutoZoom) {
      _tcscat(Scale,TEXT(" A"));
    }
    if (EnablePan) {
      _tcscat(Scale,TEXT(" PAN"));
    }
    if (EnableAuxiliaryInfo) {
      _tcscat(Scale,TEXT(" AUX"));
    }
    if (ReplayLogger::IsEnabled()) {
      _tcscat(Scale,TEXT(" REPLAY"));
    }
    if (BallastTimerActive) {
      _stprintf(TEMP,TEXT(" BALLAST %3.0f LITERS"), WEIGHTS[2]*BALLAST);
      _tcscat(Scale, TEMP);
    }
    TCHAR Buffer[20];
    RASP.ItemLabel(RasterTerrain::render_weather, Buffer);
    if (_tcslen(Buffer)) {
      _tcscat(Scale,TEXT(" "));
      _tcscat(Scale, Buffer);
    }

    SIZE tsize;
    GetTextExtentPoint(hDC, Scale, _tcslen(Scale), &tsize);

    COLORREF whitecolor = RGB(0xd0,0xd0, 0xd0);
    COLORREF blackcolor = RGB(0x20,0x20, 0x20);
    COLORREF origcolor = SetTextColor(hDC, whitecolor);

    SetTextColor(hDC, whitecolor);
    ExtTextOut(hDC, rc.right-IBLSCALE(11)-tsize.cx, End.y+IBLSCALE(8), 0,
               NULL, Scale, _tcslen(Scale), NULL);

    SetTextColor(hDC, blackcolor);
    ExtTextOut(hDC, rc.right-IBLSCALE(10)-tsize.cx, End.y+IBLSCALE(7), 0,
               NULL, Scale, _tcslen(Scale), NULL);

    #ifdef DRAWLOAD
    SelectObject(hDC, MapWindowFont);
    _stprintf(Scale,TEXT("            %d %d ms"), timestats_av,
              misc_tick_count);
    ExtTextOut(hDC, rc.left, rc.top, 0, NULL, Scale, _tcslen(Scale), NULL);
    #endif

    // restore original color
    SetTextColor(hDC, origcolor);

    SelectObject(hDC, hpOld);

  }
  if (Appearance.MapScale == apMsAltA){

    static int LastMapWidth = 0;
    double MapWidth;
    TCHAR ScaleInfo[80];
    TCHAR TEMP[20];

    HFONT          oldFont;
    int            Height;
    SIZE           TextSize;
    HBRUSH         oldBrush;
    HPEN           oldPen;
    COLORREF       oldTextColor;
    HBITMAP        oldBitMap;
    Units_t        Unit;

    if (ScaleChangeFeedback)
      MapWidth = (RequestMapScale * rc.right)/DISTANCEMODIFY/GetMapResolutionFactor();
    else
      MapWidth = (MapScale * rc.right)/DISTANCEMODIFY/GetMapResolutionFactor();

    oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
    Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo,
                              sizeof(ScaleInfo)/sizeof(TCHAR));
    GetTextExtentPoint(hDC, ScaleInfo, _tcslen(ScaleInfo), &TextSize);
    LastMapWidth = (int)MapWidth;

    Height = Appearance.MapWindowBoldFont.CapitalHeight+IBLSCALE(2);
    // 2: add 1pix border

    oldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    oldPen = (HPEN)SelectObject(hDC, GetStockObject(WHITE_PEN));
    Rectangle(hDC, 0, rc.bottom-Height,
              TextSize.cx + IBLSCALE(21), rc.bottom);
    if (ScaleChangeFeedback){
      SetBkMode(hDC, TRANSPARENT);
      oldTextColor = SetTextColor(hDC, RGB(0xff,0,0));
    }else
      oldTextColor = SetTextColor(hDC, RGB(0,0,0));

    ExtTextOut(hDC, IBLSCALE(7),
               rc.bottom-Appearance.MapWindowBoldFont.AscentHeight-IBLSCALE(1),
               0, NULL, ScaleInfo, _tcslen(ScaleInfo), NULL);

    oldBitMap = (HBITMAP)SelectObject(hDCTemp, hBmpMapScale);

    DrawBitmapX(hDC, 0, rc.bottom-Height, 6, 11, hDCTemp, 0, 0, SRCCOPY);
    DrawBitmapX(hDC,
           IBLSCALE(14)+TextSize.cx,
           rc.bottom-Height, 8, 11, hDCTemp, 6, 0, SRCCOPY);

    if (!ScaleChangeFeedback){
      HBITMAP Bmp;
      POINT   BmpPos, BmpSize;

      if (Units::GetUnitBitmap(Unit, &Bmp, &BmpPos, &BmpSize, 0)){
        HBITMAP oldBitMapa = (HBITMAP)SelectObject(hDCTemp, Bmp);

        DrawBitmapX(hDC,
                    IBLSCALE(8)+TextSize.cx, rc.bottom-Height,
                    BmpSize.x, BmpSize.y,
                    hDCTemp, BmpPos.x, BmpPos.y, SRCCOPY);
        SelectObject(hDCTemp, oldBitMapa);
      }
    }

    int y = rc.bottom-Height-
      (Appearance.TitleWindowFont.AscentHeight+IBLSCALE(2));
    if (!ScaleChangeFeedback){
      // bool FontSelected = false;
      // TODO code: gettext these
      ScaleInfo[0] = 0;
      if (AutoZoom) {
        _tcscat(ScaleInfo, TEXT("AUTO "));
      }
      if (TargetPan) {
        _tcscat(ScaleInfo, TEXT("TARGET "));
      } else if (EnablePan) {
        _tcscat(ScaleInfo, TEXT("PAN "));
      }
      if (EnableAuxiliaryInfo) {
        _tcscat(ScaleInfo, TEXT("AUX "));
      }
      if (ReplayLogger::IsEnabled()) {
        _tcscat(ScaleInfo, TEXT("REPLAY "));
      }
      if (BallastTimerActive) {
        _stprintf(TEMP,TEXT("BALLAST %3.0f LITERS"), WEIGHTS[2]*BALLAST);
        _tcscat(ScaleInfo, TEMP);
      }
      TCHAR Buffer[20];
      RASP.ItemLabel(RasterTerrain::render_weather, Buffer);
      if (_tcslen(Buffer)) {
        _tcscat(ScaleInfo, Buffer);
      }

      if (ScaleInfo[0]) {
        SelectObject(hDC, TitleWindowFont);
        // FontSelected = true;
        ExtTextOut(hDC, IBLSCALE(1), y, 0, NULL, ScaleInfo,
                   _tcslen(ScaleInfo), NULL);
        y -= (Appearance.TitleWindowFont.CapitalHeight+IBLSCALE(1));
      }
    }

    #ifdef DRAWLOAD
    SelectObject(hDC, MapWindowFont);
    _stprintf(ScaleInfo,TEXT("    %d %d ms"),
              timestats_av,
              misc_tick_count);

    ExtTextOut(hDC, rc.left, rc.top, 0, NULL, ScaleInfo,
               _tcslen(ScaleInfo), NULL);
    #endif

    SetTextColor(hDC, oldTextColor);
    SelectObject(hDC, oldPen);
    SelectObject(hDC, oldFont);
    SelectObject(hDC, oldBrush);
    SelectObject(hDCTemp, oldBitMap);

  }

}


void MapWindow::DrawGlideThroughTerrain(HDC hDC, const RECT rc) {
  HPEN hpOld;

  hpOld = (HPEN)SelectObject(hDC,
                             hpTerrainLineBg);  //sjt 02feb06 added bg line

  SelectObject(hDC,hpTerrainLineBg);
  _Polyline(hDC,Groundline,NUMTERRAINSWEEPS+1, rc);
  if ((FinalGlideTerrain==1) ||
      ((!EnableTerrain || !DerivedDrawInfo.Flying) && (FinalGlideTerrain==2))) {
    SelectObject(hDC,hpTerrainLine);
    _Polyline(hDC,Groundline,NUMTERRAINSWEEPS+1, rc);
  }

  if (DerivedDrawInfo.Flying && ValidTaskPoint(ActiveWayPoint)) {
    if ((DerivedDrawInfo.TerrainWarningLatitude != 0.0)
        &&(DerivedDrawInfo.TerrainWarningLongitude != 0.0)) {

      POINT sc;
      if (PointVisible(DerivedDrawInfo.TerrainWarningLongitude,
                       DerivedDrawInfo.TerrainWarningLatitude)) {
        LatLon2Screen(DerivedDrawInfo.TerrainWarningLongitude,
                      DerivedDrawInfo.TerrainWarningLatitude, sc);
        DrawBitmapIn(hDC, sc, hTerrainWarning);
      }
    }
  }

  SelectObject(hDC, hpOld);

}

void MapWindow::DrawBestCruiseTrack(HDC hdc, const POINT Orig)
{
  HPEN hpOld;
  HBRUSH hbOld;

  if (ActiveWayPoint<0) {
    return; // nothing to draw..
  }
  if (!ValidTaskPoint(ActiveWayPoint)) {
    return;
  }

  if (DerivedDrawInfo.WaypointDistance < 0.010)
    return;

  hpOld = (HPEN)SelectObject(hdc, hpBestCruiseTrack);
  hbOld = (HBRUSH)SelectObject(hdc, hbBestCruiseTrack);

  if (Appearance.BestCruiseTrack == ctBestCruiseTrackDefault){

    int dy = (long)(70);
    POINT Arrow[7] = { {-1,-40}, {1,-40}, {1,0}, {6,8}, {-6,8}, {-1,0}, {-1,-40}};

    Arrow[2].y -= dy;
    Arrow[3].y -= dy;
    Arrow[4].y -= dy;
    Arrow[5].y -= dy;

    PolygonRotateShift(Arrow, 7, Orig.x, Orig.y,
                       DerivedDrawInfo.BestCruiseTrack-DisplayAngle);

    Polygon(hdc,Arrow,7);

  } else
  if (Appearance.BestCruiseTrack == ctBestCruiseTrackAltA){

    POINT Arrow[] = { {-1,-40}, {-1,-62}, {-6,-62}, {0,-70}, {6,-62}, {1,-62}, {1,-40}, {-1,-40}};

    PolygonRotateShift(Arrow, sizeof(Arrow)/sizeof(Arrow[0]),
                       Orig.x, Orig.y,
                       DerivedDrawInfo.BestCruiseTrack-DisplayAngle);
    Polygon(hdc, Arrow, (sizeof(Arrow)/sizeof(Arrow[0])));
  }

  SelectObject(hdc, hpOld);
  SelectObject(hdc, hbOld);
}
