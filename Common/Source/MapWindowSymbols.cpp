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
#include "InfoBoxLayout.h"
#include "Screen/Util.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Math/Screen.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Logger.h"
#include "Dialogs.h"
#include "McReady.h"
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include "SettingsComputer.hpp"
#include <stdlib.h>

extern BOOL extGPSCONNECT;

void MapWindow::DrawCrossHairs(HDC hdc, const POINT Orig,
			       const RECT rc)
{
  POINT o1, o2;

  o1.x = Orig.x+20;
  o2.x = Orig.x-20;
  o1.y = Orig.y;
  o2.y = Orig.y;

  DrawDashLine(hdc, 1, o1, o2,
               RGB(50,50,50), rc);

  o1.x = Orig.x;
  o2.x = Orig.x;
  o1.y = Orig.y+20;
  o2.y = Orig.y-20;

  DrawDashLine(hdc, 1, o1, o2,
               RGB(50,50,50), rc);

}


void MapWindow::DrawAircraft(HDC hdc, const POINT Orig)
{

  if (Appearance.Aircraft == afAircraftDefault){

#define NUMAIRCRAFTPOINTS 16

    POINT Aircraft[NUMAIRCRAFTPOINTS] = {
      { 1,-6},
      {2,-1},
      {15,0},
      {15,2},
      {1,2},
      {0,10},
      {4,11},
      {4,12},
      {-4,12},
      {-4,11},
      {0,10},
      {-1,2},
      {-15,2},
      {-15,0},
      {-2,-1},
      {-1,-6}
    };

    int i;
    HPEN hpOld;
    HBRUSH hbAircraftSolid;
    HBRUSH hbAircraftSolidBg;

    if (Appearance.InverseAircraft) {
      hbAircraftSolid = (HBRUSH) CreateSolidBrush(RGB(0xff,0xff,0xff));
      hbAircraftSolidBg = (HBRUSH) CreateSolidBrush(RGB(0x00,0x00,0x00));
    } else {
      hbAircraftSolid = (HBRUSH) CreateSolidBrush(RGB(0x00,0x00,0x00));
      hbAircraftSolidBg = (HBRUSH) CreateSolidBrush(RGB(0xff,0xff,0xff));
    }

    HBRUSH hbOld = (HBRUSH)SelectObject(hdc, hbAircraftSolidBg);
    hpOld = (HPEN)SelectObject(hdc, MapGfx.hpAircraft);

    PolygonRotateShift(Aircraft, NUMAIRCRAFTPOINTS, Orig.x+1, Orig.y+1,
                       DisplayAircraftAngle+
                       (DerivedDrawInfo.Heading-DrawInfo.TrackBearing));

    Polygon(hdc, Aircraft, NUMAIRCRAFTPOINTS);

    // draw it again so can get white border
    SelectObject(hdc, MapGfx.hpAircraftBorder);
    SelectObject(hdc, hbAircraftSolid);

    for(i=0; i<NUMAIRCRAFTPOINTS; i++)
      {
	Aircraft[i].x -= 1;  Aircraft[i].y -= 1;
      }

    Polygon(hdc, Aircraft, NUMAIRCRAFTPOINTS);

    SelectObject(hdc, hpOld);
    SelectObject(hdc, hbOld);

    DeleteObject(hbAircraftSolid);
    DeleteObject(hbAircraftSolidBg);

  } else

    if (Appearance.Aircraft == afAircraftAltA){

      HPEN oldPen;
      POINT Aircraft[] = {
	{1, -5},
	{1, 0},
	{14, 0},
	{14, 1},
	{1, 1},
	{1, 8},
	{4, 8},
	{4, 9},
	{-3, 9},
	{-3, 8},
	{0, 8},
	{0, 1},
	{-13, 1},
	{-13, 0},
	{0, 0},
	{0, -5},
	{1, -5},
      };

      /* Experiment, when turning show the high wing larger,
	 low wing smaller
	 if (DerivedDrawInfo.TurnRate>10) {
	 Aircraft[3].y = 0;
	 Aircraft[12].y = 2;
	 } else if (DerivedDrawInfo.TurnRate<-10) {
	 Aircraft[3].y = 2;
	 Aircraft[12].y = 0;
	 }
      */

      int n = sizeof(Aircraft)/sizeof(Aircraft[0]);

      double angle = DisplayAircraftAngle+
	(DerivedDrawInfo.Heading-DrawInfo.TrackBearing);

      PolygonRotateShift(Aircraft, n,
			 Orig.x-1, Orig.y, angle);

      oldPen = (HPEN)SelectObject(hdc, MapGfx.hpAircraft);
      Polygon(hdc, Aircraft, n);

      HBRUSH hbOld;
      if (Appearance.InverseAircraft) {
	hbOld = (HBRUSH)SelectObject(hdc, GetStockObject(WHITE_BRUSH));
      } else {
	hbOld = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
      }
      SelectObject(hdc, MapGfx.hpAircraftBorder); // hpBearing
      Polygon(hdc, Aircraft, n);

      SelectObject(hdc, oldPen);
      SelectObject(hdc, hbOld);

    }

}


void MapWindow::DrawGPSStatus(HDC hDC, const RECT rc)
{

  if (extGPSCONNECT && !(DrawInfo.NAVWarning) && (DrawInfo.SatellitesUsed != 0))
    // nothing to do
    return;

  TCHAR gpswarningtext1[] = TEXT("GPS not connected");
  TCHAR gpswarningtext2[] = TEXT("GPS waiting for fix");
  TextInBoxMode_t TextInBoxMode = {2};

  if (!extGPSCONNECT) {
    SelectObject(hDCTemp,MapGfx.hGPSStatus2);
    DrawBitmapX(hDC,
                rc.left+IBLSCALE(2),
                rc.bottom+IBLSCALE(Appearance.GPSStatusOffset.y-22),
                20, 20,
                hDCTemp,
                0, 0, SRCAND);

    TextInBox(hDC, gettext(gpswarningtext1),
              rc.left+IBLSCALE(24),
              rc.bottom+IBLSCALE(Appearance.GPSStatusOffset.y-19),
              0, TextInBoxMode);

  } else
    if (DrawInfo.NAVWarning || (DrawInfo.SatellitesUsed == 0)) {
      SelectObject(hDCTemp,MapGfx.hGPSStatus1);

      DrawBitmapX(hDC,
                  rc.left+IBLSCALE(2),
                  rc.bottom+IBLSCALE(Appearance.GPSStatusOffset.y-22),
                  20, 20,
                  hDCTemp,
                  0, 0, SRCAND);

      TextInBox(hDC, gettext(gpswarningtext2),
                rc.left+IBLSCALE(24),
                rc.bottom+
                IBLSCALE(Appearance.GPSStatusOffset.y-19),
                0, TextInBoxMode);

    }

}


void MapWindow::DrawFlightMode(HDC hdc, const RECT rc)
{
  static bool flip= true;
  static double LastTime = 0;
  bool drawlogger = true;
  static bool lastLoggerActive=false;
  int offset = -1;

  if (!Appearance.DontShowLoggerIndicator){

    // has GPS time advanced?
    if(DrawInfo.Time <= LastTime) {
      LastTime = DrawInfo.Time;
    } else {
      flip = !flip;

      // don't bother drawing logger if not active for more than one second
      if ((!LoggerActive)&&(!lastLoggerActive)) {
        drawlogger = false;
      }
      lastLoggerActive = LoggerActive;
    }

    if (drawlogger) {
      offset -= 7;

      if (LoggerActive && flip) {
        SelectObject(hDCTemp,MapGfx.hLogger);
      } else {
        SelectObject(hDCTemp,MapGfx.hLoggerOff);
      }
      //changed draw mode & icon for higher opacity 12aug -st
      DrawBitmapX(hdc,
                  rc.right+IBLSCALE(offset+Appearance.FlightModeOffset.x),
                  rc.bottom+IBLSCALE(-7+Appearance.FlightModeOffset.y),
                  7,7,
                  hDCTemp,
                  0,0,SRCPAINT);

      DrawBitmapX(hdc,
                  rc.right+IBLSCALE(offset+Appearance.FlightModeOffset.x),
                  rc.bottom+IBLSCALE(-7+Appearance.FlightModeOffset.y),
                  7,7,
                  hDCTemp,
                  7,0,SRCAND);
    }
  }

  if (Appearance.FlightModeIcon == apFlightModeIconDefault){

    if (TaskAborted) {
      SelectObject(hDCTemp,MapGfx.hAbort);
    } else {
      if (DisplayMode == dmCircling) {
        SelectObject(hDCTemp,MapGfx.hClimb);
      } else if (DisplayMode == dmFinalGlide) {
        SelectObject(hDCTemp,MapGfx.hFinalGlide);
      } else {
        SelectObject(hDCTemp,MapGfx.hCruise);
      }
    }
    // Code already commented as of 12aug05 - redundant? -st
    //          BitBlt(hdc,rc.right-35,5,24,20,
    //                           hDCTemp,20,0,SRCAND);

    // code for pre 12aug icons - st
    //BitBlt(hdc,rc.right-24-3,rc.bottom-20-3,24,20,
    //  hDCTemp,0,0,SRCAND);

    offset -= 24;

    DrawBitmapX(hdc,
                rc.right+IBLSCALE(offset-1+Appearance.FlightModeOffset.x),
                rc.bottom+IBLSCALE(-20-1+Appearance.FlightModeOffset.y),
                24,20,
                hDCTemp,
                0,0,SRCPAINT);

    DrawBitmapX(hdc,
                rc.right+IBLSCALE(offset-1+Appearance.FlightModeOffset.x),
                rc.bottom+IBLSCALE(-20-1+Appearance.FlightModeOffset.y),
                24,20,
                hDCTemp,
                24,0,SRCAND);

  } else if (Appearance.FlightModeIcon == apFlightModeIconAltA){

#define SetPoint(Idx,X,Y) Arrow[Idx].x = X; Arrow[Idx].y = Y

    POINT Arrow[3];
    POINT Center;
    HBRUSH oldBrush;
    HPEN   oldPen;

    Center.x = rc.right-10;
    Center.y = rc.bottom-10;

    if (DisplayMode == dmCircling) {

      SetPoint(0,
               Center.x,
               Center.y-IBLSCALE(4));
      SetPoint(1,
               Center.x-IBLSCALE(8),
               Center.y+IBLSCALE(4));
      SetPoint(2,
               Center.x+IBLSCALE(8),
               Center.y+IBLSCALE(4));

    } else if (DisplayMode == dmFinalGlide) {

      SetPoint(0,
               Center.x,
               Center.y+IBLSCALE(4));
      SetPoint(1,
               Center.x-IBLSCALE(8),
               Center.y-IBLSCALE(4));
      SetPoint(2,
               Center.x+IBLSCALE(8),
               Center.y-IBLSCALE(4));
    } else {

      SetPoint(0,
               Center.x+IBLSCALE(4),
               Center.y);
      SetPoint(1,
               Center.x-IBLSCALE(4),
               Center.y+IBLSCALE(8));
      SetPoint(2,
               Center.x-IBLSCALE(4),
               Center.y-IBLSCALE(8));

    }

    if (TaskAborted)
      oldBrush = (HBRUSH)SelectObject(hdc, MapGfx.hBrushFlyingModeAbort);
    else
      oldBrush = (HBRUSH)SelectObject(hdc, MapGfx.hbCompass);

    oldPen = (HPEN)SelectObject(hdc, MapGfx.hpCompassBorder);
    Polygon(hdc, Arrow, 3);

    SelectObject(hdc, MapGfx.hpCompass);
    Polygon(hdc, Arrow, 3);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);

  }


  if (!Appearance.DontShowAutoMacCready && DerivedDrawInfo.AutoMacCready) {
    SelectObject(hDCTemp,MapGfx.hAutoMacCready);

    offset -= 24;

    //changed draw mode & icon for higher opacity 12aug -st

    DrawBitmapX(hdc,
		rc.right+IBLSCALE(offset-3+Appearance.FlightModeOffset.x),
		rc.bottom+IBLSCALE(-20-3+Appearance.FlightModeOffset.y),
		24,20,
		hDCTemp,
		0,0,SRCPAINT);

    DrawBitmapX(hdc,
		rc.right+IBLSCALE(offset-3+Appearance.FlightModeOffset.x),
		rc.bottom+IBLSCALE(-20-3+Appearance.FlightModeOffset.y),
		24,20,
		hDCTemp,
		24,0,SRCAND);

    //  commented @ 12aug st
    //  BitBlt(hdc,rc.right-48-3,rc.bottom-20-3,24,20,
    //    hDCTemp,0,0,SRCAND);
  };

}



void MapWindow::DrawWindAtAircraft2(HDC hdc, const POINT Orig, const RECT rc) {
  int i;
  POINT Start;
  HPEN hpOld;
  HBRUSH hbOld;
  TCHAR sTmp[12];
  static SIZE tsize = {0,0};

  if (DerivedDrawInfo.WindSpeed<1) {
    return; // JMW don't bother drawing it if not significant
  }

  if (tsize.cx == 0){

    HFONT oldFont = (HFONT)SelectObject(hdc, MapWindowBoldFont);
    GetTextExtentPoint(hdc, TEXT("99"), 2, &tsize);
    SelectObject(hdc, oldFont);
    tsize.cx = tsize.cx/2;
  }

  hpOld = (HPEN)SelectObject(hdc, MapGfx.hpWind);
  hbOld = (HBRUSH)SelectObject(hdc, MapGfx.hbWind);

  int wmag = iround(4.0*DerivedDrawInfo.WindSpeed);

  Start.y = Orig.y;
  Start.x = Orig.x;

  int kx = tsize.cx/InfoBoxLayout::scale/2;

  POINT Arrow[7] = { {0,-20}, {-6,-26}, {0,-20},
                     {6,-26}, {0,-20},
                     {8+kx, -24},
                     {-8-kx, -24}};

  for (i=1;i<4;i++)
    Arrow[i].y -= wmag;

  PolygonRotateShift(Arrow, 7, Start.x, Start.y,
		     DerivedDrawInfo.WindBearing-DisplayAngle);
  Polygon(hdc, Arrow, 5);

  if (WindArrowStyle==1) {
    POINT Tail[2] = {{0,-20}, {0,-26-min(20,wmag)*3}};
    double angle = AngleLimit360(DerivedDrawInfo.WindBearing-DisplayAngle);
    for(i=0; i<2; i++) {
      if (InfoBoxLayout::scale>1) {
        Tail[i].x *= InfoBoxLayout::scale;
        Tail[i].y *= InfoBoxLayout::scale;
      }
      protateshift(Tail[i], angle, Start.x, Start.y);
    }

    // optionally draw dashed line
    ClipDrawLine(hdc, PS_DASH, 1, Tail[0], Tail[1], RGB(0,0,0), rc);
  }


  _stprintf(sTmp, TEXT("%i"), iround(DerivedDrawInfo.WindSpeed * SPEEDMODIFY));

  TextInBoxMode_t TextInBoxMode = { 16 | 32 }; // JMW test {2 | 16};
  if (Arrow[5].y>=Arrow[6].y) {
    TextInBox(hdc, sTmp, Arrow[5].x-kx, Arrow[5].y, 0, TextInBoxMode);
  } else {
    TextInBox(hdc, sTmp, Arrow[6].x-kx, Arrow[6].y, 0, TextInBoxMode);
  }

  SelectObject(hdc, hbOld);
  SelectObject(hdc, hpOld);
}


/////////////////


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

  ClipDrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0));
  */

  a1.x = Start.x+radius/2;
  a1.y = Start.y;
  a2.x = Start.x-radius/2;
  a2.y = Start.y;
  ClipDrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, a2, RGB(0,0,0), rc);

  a1.x = Start.x;
  a1.y = Start.y-radius/4;
  ClipDrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, Start, RGB(0,0,0), rc);

  //

#define ROOT2 0.70711

  int rr2p = lround(radius*ROOT2+IBLSCALE(1));
  int rr2n = lround(radius*ROOT2);

  a1.x = Start.x+rr2p;
  a1.y = Start.y-rr2p;
  a2.x = Start.x+rr2n;
  a2.y = Start.y-rr2n;

  ClipDrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0), rc);

  a1.x = Start.x-rr2p;
  a1.y = Start.y-rr2p;
  a2.x = Start.x-rr2n;
  a2.y = Start.y-rr2n;

  ClipDrawLine(hDC, PS_SOLID, IBLSCALE(1),
            a1, a2, RGB(0,0,0), rc);

  // JMW experimental, display stall sensor
  double s = max(0.0,min(1.0,DrawInfo.StallRatio));
  long m = (long)((rc.bottom-rc.top)*s*s);
  a1.x = rc.right-1;
  a1.y = rc.bottom-m;
  a2.x = a1.x-10;
  a2.y = a1.y;
  ClipDrawLine(hDC, PS_SOLID, IBLSCALE(2),
            a1, a2, RGB(0xff,0,0), rc);

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);
  DeleteObject((HPEN)hpHorizonSky);
  DeleteObject((HBRUSH)hbHorizonSky);
  DeleteObject((HPEN)hpHorizonGround);
  DeleteObject((HBRUSH)hbHorizonGround);
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

  mutexTaskData.Lock();  // protect from external task changes
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
          hpOld = (HPEN)SelectObject(hDC, MapGfx.hpFinalGlideBelowLandable);
          hbOld = (HBRUSH)SelectObject(hDC, MapGfx.hbFinalGlideBelowLandable);
        } else {
          hpOld = (HPEN)SelectObject(hDC, MapGfx.hpFinalGlideBelow);
          hbOld = (HBRUSH)SelectObject(hDC, MapGfx.hbFinalGlideBelow);
        }
      } else {
        hpOld = (HPEN)SelectObject(hDC, MapGfx.hpFinalGlideAbove);
        hbOld = (HBRUSH)SelectObject(hDC, MapGfx.hbFinalGlideAbove);
      }
      Polygon(hDC,GlideBar,6);

      // draw glide bar at mc 0
      if (Offset0<=0) {
        if (LandableReachable) {
          SelectObject(hDC, MapGfx.hpFinalGlideBelowLandable);
          SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
        } else {
          SelectObject(hDC, MapGfx.hpFinalGlideBelow);
          SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
        }
      } else {
        SelectObject(hDC, MapGfx.hpFinalGlideAbove);
        SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
      }
      if (Offset!=Offset0) {
        Polygon(hDC,GlideBar0,6);
      }

      // JMW draw x on final glide bar if unreachable at current Mc
      // hpAircraftBorder
      if ((DerivedDrawInfo.TaskTimeToGo>0.9*ERROR_TIME)
	  || ((MACCREADY<0.01) && (DerivedDrawInfo.TaskAltitudeDifference<0))) {
	SelectObject(hDC, MapGfx.hpAircraftBorder);
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
       mutexTaskData.Unlock();
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

    hpOld = (HPEN)SelectObject(hDC, MapGfx.hpCompass);
    hbOld = (HBRUSH)SelectObject(hDC, MapGfx.hbCompass);

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

    hpOld = (HPEN)SelectObject(hDC, MapGfx.hpCompassBorder);
    hbOld = (HBRUSH)SelectObject(hDC, MapGfx.hbCompass);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, MapGfx.hpCompass);
    Polygon(hDC,Arrow,5);

    SelectObject(hDC, hbOld);
    SelectObject(hDC, hpOld);

  }

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

  hpOld = (HPEN)SelectObject(hdc, MapGfx.hpBestCruiseTrack);
  hbOld = (HBRUSH)SelectObject(hdc, MapGfx.hbBestCruiseTrack);

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

      ClipPolyline(hDC, chevron, 3, rc);
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
  ClipPolyline(hDC, chevron, 2, rc);

  SelectObject(hDC, hbOld);
  SelectObject(hDC, hpOld);

}
*/


#include "GaugeCDI.h"

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
