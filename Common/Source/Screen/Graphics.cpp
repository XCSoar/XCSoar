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

#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Util.hpp"
#include "Appearance.hpp"
#include "MapWindowProjection.hpp"
#include "Interface.hpp"
#include "InfoBoxLayout.h"
#include "Math/Screen.hpp"
#include <stdlib.h>
#include "SettingsUser.hpp"
#include "SettingsAirspace.hpp"

#define NUMSNAILRAMP 6

const COLORRAMP snail_colors[] = {
  {0,         0xff, 0x3e, 0x00},
  {50,        0xcd, 0x4f, 0x27},
  {100,       0x8f, 0x8f, 0x8f},
  {150,       0x27, 0xcd, 0x4f},
  {201,       0x00, 0xff, 0x3e},
  {501,       0x00, 0xff, 0x3e}
};


// airspace brushes/colours
COLORREF ScreenGraphics::GetAirspaceColour(const int i) {
  return Colours[i];
}
HBRUSH ScreenGraphics::GetAirspaceBrush(const int i) {
  return hAirspaceBrushes[i];
}
COLORREF ScreenGraphics::GetAirspaceColourByClass(const int i) {
  return Colours[iAirspaceColour[i]];
}
HBRUSH ScreenGraphics::GetAirspaceBrushByClass(const int i) {
  return hAirspaceBrushes[iAirspaceBrush[i]];
}

const COLORREF ScreenGraphics::ColorSelected = RGB(0xC0,0xC0,0xC0);
const COLORREF ScreenGraphics::ColorUnselected = RGB(0xFF,0xFF,0xFF);
const COLORREF ScreenGraphics::ColorWarning = RGB(0xFF,0x00,0x00);
const COLORREF ScreenGraphics::ColorOK = RGB(0x00,0x00,0xFF);
const COLORREF ScreenGraphics::ColorButton = RGB(0xA0,0xE0,0xA0);
const COLORREF ScreenGraphics::ColorBlack = RGB(0x00,0x00,0x00);
const COLORREF ScreenGraphics::ColorMidGrey = RGB(0x80,0x80,0x80);

const COLORREF ScreenGraphics::redColor = RGB(0xff,0x00,0x00);
const COLORREF ScreenGraphics::blueColor = RGB(0x00,0x00,0xff);
const COLORREF ScreenGraphics::inv_redColor = RGB(0xff,0x70,0x70);
const COLORREF ScreenGraphics::inv_blueColor = RGB(0x90,0x90,0xff);
const COLORREF ScreenGraphics::yellowColor = RGB(0xff,0xff,0x00);//VENTA2
const COLORREF ScreenGraphics::greenColor = RGB(0x00,0xff,0x00);//VENTA2
const COLORREF ScreenGraphics::magentaColor = RGB(0xff,0x00,0xff);//VENTA2
const COLORREF ScreenGraphics::inv_yellowColor = RGB(0xff,0xff,0x00); //VENTA2
const COLORREF ScreenGraphics::inv_greenColor = RGB(0x00,0xff,0x00); //VENTA2
const COLORREF ScreenGraphics::inv_magentaColor = RGB(0xff,0x00,0xff); //VENTA2
const COLORREF ScreenGraphics::TaskColor = RGB(0,120,0); // was 255
const COLORREF ScreenGraphics::BackgroundColor = RGB(0xFF,0xFF,0xFF);
const COLORREF ScreenGraphics::Colours[] =
{
  RGB(0xFF,0x00,0x00),
  RGB(0x00,0xFF,0x00),
  RGB(0x00,0x00,0xFF),
  RGB(0xFF,0xFF,0x00),
  RGB(0xFF,0x00,0xFF),
  RGB(0x00,0xFF,0xFF),
  RGB(0x7F,0x00,0x00),
  RGB(0x00,0x7F,0x00),
  RGB(0x00,0x00,0x7F),
  RGB(0x7F,0x7F,0x00),
  RGB(0x7F,0x00,0x7F),
  RGB(0x00,0x7F,0x7F),
  RGB(0xFF,0xFF,0xFF),
  RGB(0xC0,0xC0,0xC0),
  RGB(0x7F,0x7F,0x7F),
  RGB(0x00,0x00,0x00),
};

void ScreenGraphics::Initialise(void) {
  int i;

  redBrush = CreateSolidBrush(redColor);
  yellowBrush = CreateSolidBrush(yellowColor);
  greenBrush = CreateSolidBrush(greenColor);

  hBackgroundBrush = CreateSolidBrush(BackgroundColor);

  hFLARMTraffic=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FLARMTRAFFIC));
  hTerrainWarning=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TERRAINWARNING));
  hTurnPoint=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TURNPOINT));
  hSmall=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SMALL));
  hAutoMacCready=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AUTOMCREADY));
  hGPSStatus1=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GPSSTATUS1));
  hGPSStatus2=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GPSSTATUS2));
  hLogger=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGGER));
  hLoggerOff=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGGEROFF));
  hBmpTeammatePosition = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TEAMMATE_POS));

  hCruise=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CRUISE));
  hClimb=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMB));
  hFinalGlide=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FINALGLIDE));
  hAbort=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ABORT));

  // airspace brushes and colours

  hAirspaceBitmap[0]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE0));
  hAirspaceBitmap[1]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE1));
  hAirspaceBitmap[2]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE2));
  hAirspaceBitmap[3]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE3));
  hAirspaceBitmap[4]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE4));
  hAirspaceBitmap[5]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE5));
  hAirspaceBitmap[6]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE6));
  hAirspaceBitmap[7]=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRSPACE7));

  hAboveTerrainBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ABOVETERRAIN));

  for (i=0; i<NUMAIRSPACEBRUSHES; i++) {
    hAirspaceBrushes[i] =
      CreatePatternBrush((HBITMAP)hAirspaceBitmap[i]);
  }
  hAboveTerrainBrush = CreatePatternBrush((HBITMAP)hAboveTerrainBitmap);

  BYTE Red,Green,Blue;
  int iwidth;
  int minwidth;
  minwidth = max(IBLSCALE(2),IBLSCALE(SnailWidthScale)/16);
  for (i=0; i<NUMSNAILCOLORS; i++) {
    short ih = i*200/(NUMSNAILCOLORS-1);
    ColorRampLookup(ih,
		    Red, Green, Blue,
		    snail_colors, NUMSNAILRAMP, 6);
    if (i<NUMSNAILCOLORS/2) {
      iwidth= minwidth;
    } else {
      iwidth = max(minwidth,
		   (i-NUMSNAILCOLORS/2)
		   *IBLSCALE(SnailWidthScale)/NUMSNAILCOLORS);
    }

    hSnailColours[i] = RGB((BYTE)Red,(BYTE)Green,(BYTE)Blue);
    hSnailPens[i] = (HPEN)CreatePen(PS_SOLID, iwidth, hSnailColours[i]);

  }

  /* JMW created all re-used pens here */

  hpCompassBorder = (HPEN)CreatePen(PS_SOLID, IBLSCALE(3),
				    RGB(0xff,0xff,0xff));

  // testing only    Appearance.InverseAircraft = true;

  if (Appearance.InverseAircraft) {
    hpAircraft = (HPEN)CreatePen(PS_SOLID, IBLSCALE(3),
				 RGB(0x00,0x00,0x00));
    hpAircraftBorder = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				       RGB(0xff,0xff,0xff));
  } else {
    hpAircraft = (HPEN)CreatePen(PS_SOLID, IBLSCALE(3),
				 RGB(0xff,0xff,0xff));
    hpAircraftBorder = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				       RGB(0x00,0x00,0x00));
  }

#if (MONOCHROME_SCREEN > 0)
  hpWind = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(0,0,0));
#else
  hpWind = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(255,0,0));
#endif

  hpWindThick = (HPEN)CreatePen(PS_SOLID, IBLSCALE(4),
				RGB(255,220,220));

  hpBearing = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2),
			      RGB(0,0,0));
  hpBestCruiseTrack = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				      RGB(0,0,255));
#if (MONOCHROME_SCREEN > 0)
  hpCompass = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0x00,0x00,0x00));
  //hpCompass = (HPEN)CreatePen(PS_SOLID, 1, RGB(0xff,0xff,0xff));
#else
  hpCompass = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0xcf,0xcf,0xFF));
#endif
  hpThermalBand = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(0x40,0x40,0xFF));
  hpThermalBandGlider = (HPEN)CreatePen(PS_SOLID, IBLSCALE(2), RGB(0x00,0x00,0x30));

  hpFinalGlideBelow = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0xFF,0xA0,0xA0));
  hpFinalGlideBelowLandable = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(255,196,0));

  // TODO enhancement: support red/green Color blind
  hpFinalGlideAbove = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1), RGB(0xA0,0xFF,0xA0));

  hpSpeedSlow=(HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
			      RGB(0xFF,0x00,0x00));
  hpSpeedFast=(HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
			      RGB(0x00,0xFF,0x00));

  hpStartFinishThick=(HPEN)CreatePen(PS_SOLID, IBLSCALE(5),
				     TaskColor);

  hpStartFinishThin=(HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
				    RGB(255,0,0));

  hpMapScale = (HPEN)CreatePen(PS_SOLID, IBLSCALE(1),
			       RGB(0,0,0));
  hpTerrainLine = (HPEN)CreatePen(PS_DASH, (1),
				  RGB(0x30,0x30,0x30));
  hpTerrainLineBg = (HPEN)CreatePen(PS_SOLID, (1),
				    RGB(0xFF,0xFF,0xFF));
  // VENTA3
  hpVisualGlideLightBlack = (HPEN)CreatePen(PS_DASH, (1), RGB(0x0,0x0,0x0));
  hpVisualGlideHeavyBlack = (HPEN)CreatePen(PS_DASH, (2), RGB(0x0,0x0,0x0));
  hpVisualGlideLightRed = (HPEN)CreatePen(PS_DASH, (1), RGB(0xff,0x0,0x0));
  hpVisualGlideHeavyRed = (HPEN)CreatePen(PS_DASH, (2), RGB(0xff,0x0,0x0));

#if (MONOCHROME_SCREEN > 0)
  hbCompass=(HBRUSH)CreateSolidBrush(RGB(0xff,0xff,0xff));
#else
  hbCompass=(HBRUSH)CreateSolidBrush(RGB(0x40,0x40,0xFF));
#endif
  hbThermalBand=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0xFF));
  hbBestCruiseTrack=(HBRUSH)CreateSolidBrush(RGB(0x0,0x0,0xFF));
  hbFinalGlideBelow=(HBRUSH)CreateSolidBrush(RGB(0xFF,0x00,0x00));
  hbFinalGlideBelowLandable=(HBRUSH)CreateSolidBrush(RGB(0xFF,180,0x00));
  hbFinalGlideAbove=(HBRUSH)CreateSolidBrush(RGB(0x00,0xFF,0x00));

#if (MONOCHROME_SCREEN > 0)
  hbWind=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0x80));
#else
  hbWind=(HBRUSH)CreateSolidBrush(RGB(0x80,0x80,0x80));
#endif

  hBmpMapScale = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MAPSCALE_A));

  hBrushFlyingModeAbort = (HBRUSH)CreateSolidBrush(RGB(0xff,0x00,0x00));

  if (Appearance.IndLandable == wpLandableDefault){
    hBmpAirportReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
    hBmpAirportUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
    hBmpFieldReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
    hBmpFieldUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
  }else
    if (Appearance.IndLandable == wpLandableAltA){
      hBmpAirportReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRPORT_REACHABLE));
      hBmpAirportUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AIRPORT_UNREACHABLE));
      hBmpFieldReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OUTFILED_REACHABLE));
      hBmpFieldUnReachable = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OUTFILED_UNREACHABLE));
    }

  hBmpThermalSource = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_THERMALSOURCE));
  hBmpTarget = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TARGET));

  for (int i=0; i<AIRSPACECLASSCOUNT; i++) {
    hAirspacePens[i] =
      CreatePen(PS_SOLID, IBLSCALE(2), GetAirspaceColourByClass(i));
  }

}

void ScreenGraphics::Destroy() {

  int i;

  DeleteObject(hTurnPoint);
  DeleteObject(hSmall);
  DeleteObject(hCruise);
  DeleteObject(hClimb);
  DeleteObject(hFinalGlide);
  DeleteObject(hAutoMacCready);
  DeleteObject(hFLARMTraffic);
  DeleteObject(hTerrainWarning);
  DeleteObject(hGPSStatus1);
  DeleteObject(hGPSStatus2);
  DeleteObject(hAbort);
  DeleteObject(hLogger);
  DeleteObject(hLoggerOff);

  DeleteObject((HPEN)hpAircraft);
  DeleteObject((HPEN)hpAircraftBorder);
  DeleteObject((HPEN)hpWind);
  DeleteObject((HPEN)hpWindThick);
  DeleteObject((HPEN)hpBearing);
  DeleteObject((HPEN)hpBestCruiseTrack);
  DeleteObject((HPEN)hpCompass);
  DeleteObject((HPEN)hpThermalBand);
  DeleteObject((HPEN)hpThermalBandGlider);
  DeleteObject((HPEN)hpFinalGlideAbove);
  DeleteObject((HPEN)hpFinalGlideBelow);
  DeleteObject((HPEN)hpFinalGlideBelowLandable);
  DeleteObject((HPEN)hpMapScale);
  DeleteObject((HPEN)hpTerrainLine);
  DeleteObject((HPEN)hpTerrainLineBg);
  DeleteObject((HPEN)hpSpeedFast);
  DeleteObject((HPEN)hpSpeedSlow);
  DeleteObject((HPEN)hpStartFinishThick);
  DeleteObject((HPEN)hpStartFinishThin);

  DeleteObject((HPEN)hpVisualGlideLightBlack); // VENTA3
  DeleteObject((HPEN)hpVisualGlideLightRed); // VENTA3
  DeleteObject((HPEN)hpVisualGlideHeavyRed); // VENTA3
  DeleteObject((HPEN)hpVisualGlideHeavyBlack); // VENTA3

  DeleteObject((HBRUSH)hbCompass);
  DeleteObject((HBRUSH)hbThermalBand);
  DeleteObject((HBRUSH)hbBestCruiseTrack);
  DeleteObject((HBRUSH)hbFinalGlideBelow);
  DeleteObject((HBRUSH)hbFinalGlideBelowLandable);
  DeleteObject((HBRUSH)hbFinalGlideAbove);
  DeleteObject((HBRUSH)hbWind);

  DeleteObject(hBmpMapScale);
  DeleteObject(hBmpCompassBg);
  DeleteObject(hBackgroundBrush);
  DeleteObject(hBmpClimbeAbort);

  DeleteObject((HPEN)hpCompassBorder);
  DeleteObject((HBRUSH)hBrushFlyingModeAbort);

  DeleteObject(hBmpAirportReachable);
  DeleteObject(hBmpAirportUnReachable);
  DeleteObject(hBmpFieldReachable);
  DeleteObject(hBmpFieldUnReachable);
  DeleteObject(hBmpThermalSource);
  DeleteObject(hBmpTarget);
  DeleteObject(hBmpTeammatePosition);

  for(i=0;i<NUMAIRSPACEBRUSHES;i++)
    {
      DeleteObject(hAirspaceBrushes[i]);
      DeleteObject(hAirspaceBitmap[i]);
    }

  DeleteObject(hAboveTerrainBitmap);
  DeleteObject(hAboveTerrainBrush);

  for (i=0; i<AIRSPACECLASSCOUNT; i++) {
    DeleteObject(hAirspacePens[i]);
  }

  for (i=0; i<NUMSNAILCOLORS; i++) {
    DeleteObject(hSnailPens[i]);
  }

  DeleteObject(greenBrush);
  DeleteObject(yellowBrush);
  DeleteObject(redBrush);
}


void DrawBitmapX(HDC hdc, int x, int y,
		 int sizex, int sizey,
		 HDC source,
		 int offsetx, int offsety,
		 DWORD mode) {

  if (InfoBoxLayout::scale>1) {
    StretchBlt(hdc, x, y,
               IBLSCALE(sizex),
               IBLSCALE(sizey),
               source,
               offsetx, offsety, sizex, sizey,
               mode);
  } else {
    BitBlt(hdc, x, y, sizex, sizey,
           source, offsetx, offsety, mode);
  }
}


void DrawDashLine(HDC hdc, const int width,
		  const POINT ptStart, const POINT ptEnd,
		  const COLORREF cr,
		  const RECT rc)
{
  int i;
  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  hpDash = (HPEN)CreatePen(PS_DASH, 1, cr);
  hpOld = (HPEN)SelectObject(hdc, hpDash);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;

  //increment on smallest variance
  if(abs(ptStart.x - ptEnd.x) < abs(ptStart.y - ptEnd.y)){
    pt[0].x -= width / 2;
    pt[1].x -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].x += 1;
      pt[1].x += 1;
      ClipPolyline(hdc, pt, 2, rc);
    }
  } else {
    pt[0].y -= width / 2;
    pt[1].y -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].y += 1;
      pt[1].y += 1;
      ClipPolyline(hdc, pt, 2, rc);
    }
  }

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDash);
}


void ClipDrawLine(HDC hdc, const int PenStyle, const int width,
		  const POINT ptStart, const POINT ptEnd,
		  const COLORREF cr, const RECT rc) {

  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  hpDash = (HPEN)CreatePen(PenStyle, width, cr);
  hpOld = (HPEN)SelectObject(hdc, hpDash);

  pt[0].x = ptStart.x;
  pt[0].y = ptStart.y;
  pt[1].x = ptEnd.x;
  pt[1].y = ptEnd.y;

  ClipPolyline(hdc, pt, 2, rc);

  SelectObject(hdc, hpOld);
  DeleteObject((HPEN)hpDash);
}


void DrawGreatCircle(HDC hdc,
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
    ClipLine(hdc, StartP, EndP, rc);
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

      ClipLine(hdc, StartP, EndP, rc);

      StartP.x = EndP.x;
      StartP.y = EndP.y;

      startLat = tlat1;
      startLon = tlon1;

    }
  }
#else
  // Simple and this should work for PNA with display bug

  HPEN hpOld = (HPEN)SelectObject(hdc, MapGfx.hpBearing);
  POINT pt[2];
  MapWindowProjection::LatLon2Screen(startLon,
                startLat,
                pt[0]);
  MapWindowProjection::LatLon2Screen(targetLon,
                targetLat,
                pt[1]);
  ClipPolygon(hdc, pt, 2, rc, false);

#endif
  SelectObject(hdc, hpOld);
}



bool TextInBoxMoveInView(POINT *offset, RECT *brect, const RECT &MapRect){

  bool res = false;

  int LabelMargin = 4;

  offset->x = 0;
  offset->y = 0;

  if (MapRect.top > brect->top){
    int d = MapRect.top - brect->top;
    brect->top += d;
    brect->bottom += d;
    offset->y += d;
    brect->bottom -= d;
    brect->left -= d;
    offset->x -= d;
    res = true;
  }

  if (MapRect.right < brect->right){
    int d = MapRect.right - brect->right;

    if (offset->y < LabelMargin){
      int dy;

      if (d > -LabelMargin){
        dy = LabelMargin-offset->y;
        if (d > -dy)
          dy = -d;
      } else {
        int x = d + (brect->right - brect->left) + 10;

        dy = x - offset->y;

        if (dy < 0)
          dy = 0;

        if (dy > LabelMargin)
          dy = LabelMargin;
      }

      brect->top += dy;
      brect->bottom += dy;
      offset->y += dy;

    }

    brect->right += d;
    brect->left += d;
    offset->x += d;

    res = true;
  }

  if (MapRect.bottom < brect->bottom){
    if (offset->x == 0){
      int d = MapRect.bottom - brect->bottom;
      brect->top += d;
      brect->bottom += d;
      offset->y += d;
    } else
      if (offset->x < -LabelMargin){
	int d = -(brect->bottom - brect->top) - 10;
	brect->top += d;
	brect->bottom += d;
	offset->y += d;
      } else {
	int d = -(2*offset->x + (brect->bottom - brect->top));
	brect->top += d;
	brect->bottom += d;
	offset->y += d;
      }

    res = true;
  }

  if (MapRect.left > brect->left){
    int d = MapRect.left - brect->left;
    brect->right+= d;
    brect->left += d;
    offset->x += d;
    res = true;
  }

  return(res);

}



// returns true if really wrote something
bool TextInBox(HDC hDC, const TCHAR* Value, int x, int y,
	       int size, TextInBoxMode_t Mode, bool noOverlap) {

  SIZE tsize;
  RECT brect;
  HFONT oldFont=0;
  POINT org;
  bool drawn=false;

  RECT MapRect = MapWindowProjection::GetMapRect();

  if ((x<MapRect.left-WPCIRCLESIZE) ||
      (x>MapRect.right+(WPCIRCLESIZE*3)) ||
      (y<MapRect.top-WPCIRCLESIZE) ||
      (y>MapRect.bottom+WPCIRCLESIZE)) {
    return drawn; // FIX Not drawn really
  }

  org.x = x;
  org.y = y;

  if (size==0) {
    size = _tcslen(Value);
  }

  HBRUSH hbOld;
  hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));

  if (Mode.AsFlag.Reachable){
    if (Appearance.IndLandable == wpLandableDefault){
      x += 5;  // make space for the green circle
    }else
      if (Appearance.IndLandable == wpLandableAltA){
	x += 0;
      }
  }

  // landable waypoint label inside white box
  if (!Mode.AsFlag.NoSetFont) {  // VENTA5 predefined font from calling function
    if (Mode.AsFlag.Border){
      oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
    } else {
      oldFont = (HFONT)SelectObject(hDC, MapWindowFont);
    }
  }

  GetTextExtentPoint(hDC, Value, size, &tsize);

  if (Mode.AsFlag.AlligneRight){
    x -= tsize.cx;
  } else
    if (Mode.AsFlag.AlligneCenter){
      x -= tsize.cx/2;
      y -= tsize.cy/2;
    }

  bool notoverlapping = true;

  if (Mode.AsFlag.Border || Mode.AsFlag.WhiteBorder){

    POINT offset;

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    if (Mode.AsFlag.AlligneRight)
      x -= 3;

    if (TextInBoxMoveInView(&offset, &brect, MapRect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
      HPEN oldPen;
      if (Mode.AsFlag.Border) {
        oldPen = (HPEN)SelectObject(hDC, MapGfx.hpMapScale);
      } else {
        oldPen = (HPEN)SelectObject(hDC, GetStockObject(WHITE_PEN));
      }
      RoundRect(hDC, brect.left, brect.top, brect.right, brect.bottom,
                IBLSCALE(8), IBLSCALE(8));
      SelectObject(hDC, oldPen);
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
      drawn=true;
    }


  } else if (Mode.AsFlag.FillBackground) {

    POINT offset;

    brect.left = x-1;
    brect.right = brect.left+tsize.cx+1;
    brect.top = y+((tsize.cy+4)>>3);
    brect.bottom = brect.top+tsize.cy-((tsize.cy+4)>>3);

    if (Mode.AsFlag.AlligneRight)
      x -= 2;

    if (TextInBoxMoveInView(&offset, &brect, MapRect)){
      x += offset.x;
      y += offset.y;
    }

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
      COLORREF oldColor = SetBkColor(hDC, RGB(0xff, 0xff, 0xff));
      ExtTextOut(hDC, x, y, ETO_OPAQUE, &brect, Value, size, NULL);
      SetBkColor(hDC, oldColor);
      drawn=true;
    }

  } else if (Mode.AsFlag.WhiteBold) {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
	SetTextColor(hDC,RGB(0xff,0xff,0xff));

#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x+1, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x+2, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-1, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-2, y, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y+1, 0, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y-1, 0, NULL, Value, size, NULL);
	SetTextColor(hDC,RGB(0x00,0x00,0x00));

      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);

#else
      ExtTextOut(hDC, x+2, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x+1, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-1, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x-2, y, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y+1, ETO_OPAQUE, NULL, Value, size, NULL);
      ExtTextOut(hDC, x, y-1, ETO_OPAQUE, NULL, Value, size, NULL);
	SetTextColor(hDC,RGB(0x00,0x00,0x00));

      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
      drawn=true;
    }

  } else {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
#if (WINDOWSPC>0)
      SetBkMode(hDC,TRANSPARENT);
      ExtTextOut(hDC, x, y, 0, NULL, Value, size, NULL);
#else
      ExtTextOut(hDC, x, y, ETO_OPAQUE, NULL, Value, size, NULL);
#endif
      drawn=true;
    }

  }

  SelectObject(hDC, hbOld);

  return drawn;

}

// simple code to prevent text writing over map city names

static int nLabelBlocks;
static RECT LabelBlockCoords[MAXLABELBLOCKS];

void LabelBlockReset() {
  nLabelBlocks = 0;
}


bool checkLabelBlock(RECT rc) {
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


