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
#include "InfoBoxLayout.h"
#include "Math/Screen.hpp"
#include <stdlib.h>
#include "SettingsUser.hpp"
#include "SettingsAirspace.hpp"
#include "Units.h"

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
const Color ScreenGraphics::GetAirspaceColour(const int i) {
  return Colours[i];
}
const Brush &ScreenGraphics::GetAirspaceBrush(const int i) {
  return hAirspaceBrushes[i];
}
const Color ScreenGraphics::GetAirspaceColourByClass(const int i) {
  return Colours[iAirspaceColour[i]];
}
const Brush &ScreenGraphics::GetAirspaceBrushByClass(const int i) {
  return hAirspaceBrushes[iAirspaceBrush[i]];
}


const Color ScreenGraphics::ColorSelected = Color(0xC0,0xC0,0xC0);
const Color ScreenGraphics::ColorUnselected = Color(0xFF,0xFF,0xFF);
const Color ScreenGraphics::ColorWarning = Color(0xFF,0x00,0x00);
const Color ScreenGraphics::ColorOK = Color(0x00,0x00,0xFF);
const Color ScreenGraphics::ColorButton = Color(0xA0,0xE0,0xA0);
const Color ScreenGraphics::ColorBlack = Color(0x00,0x00,0x00);
const Color ScreenGraphics::ColorMidGrey = Color(0x80,0x80,0x80);

const Color ScreenGraphics::redColor = Color(0xff,0x00,0x00);
const Color ScreenGraphics::blueColor = Color(0x00,0x00,0xff);
const Color ScreenGraphics::inv_redColor = Color(0xff,0x70,0x70);
const Color ScreenGraphics::inv_blueColor = Color(0x90,0x90,0xff);
const Color ScreenGraphics::yellowColor = Color(0xff,0xff,0x00);//VENTA2
const Color ScreenGraphics::greenColor = Color(0x00,0xff,0x00);//VENTA2
const Color ScreenGraphics::magentaColor = Color(0xff,0x00,0xff);//VENTA2
const Color ScreenGraphics::inv_yellowColor = Color(0xff,0xff,0x00); //VENTA2
const Color ScreenGraphics::inv_greenColor = Color(0x00,0xff,0x00); //VENTA2
const Color ScreenGraphics::inv_magentaColor = Color(0xff,0x00,0xff); //VENTA2
const Color ScreenGraphics::TaskColor = Color(0,120,0); // was 255
const Color ScreenGraphics::BackgroundColor = Color(0xFF,0xFF,0xFF);
const Color ScreenGraphics::Colours[] =
{
  Color(0xFF,0x00,0x00),
  Color(0x00,0xFF,0x00),
  Color(0x00,0x00,0xFF),
  Color(0xFF,0xFF,0x00),
  Color(0xFF,0x00,0xFF),
  Color(0x00,0xFF,0xFF),
  Color(0x7F,0x00,0x00),
  Color(0x00,0x7F,0x00),
  Color(0x00,0x00,0x7F),
  Color(0x7F,0x7F,0x00),
  Color(0x7F,0x00,0x7F),
  Color(0x00,0x7F,0x7F),
  Color(0xFF,0xFF,0xFF),
  Color(0xC0,0xC0,0xC0),
  Color(0x7F,0x7F,0x7F),
  Color(0x00,0x00,0x00),
};

void ScreenGraphics::Initialise(HINSTANCE hInstance) {
  int i;

  Units::LoadUnitBitmap(hInstance);

  infoSelectedBrush.set(MapGfx.ColorSelected);
  infoUnselectedBrush.set(MapGfx.ColorUnselected);
  buttonBrush.set(MapGfx.ColorButton);

  redBrush.set(redColor);
  yellowBrush.set(yellowColor);
  greenBrush.set(greenColor);

  hBackgroundBrush.set(BackgroundColor);

  hFLARMTraffic.load(IDB_FLARMTRAFFIC);
  hTerrainWarning.load(IDB_TERRAINWARNING);
  hTurnPoint.load(IDB_TURNPOINT);
  hSmall.load(IDB_SMALL);
  hAutoMacCready.load(IDB_AUTOMCREADY);
  hGPSStatus1.load(IDB_GPSSTATUS1);
  hGPSStatus2.load(IDB_GPSSTATUS2);
  hLogger.load(IDB_LOGGER);
  hLoggerOff.load(IDB_LOGGEROFF);
  hBmpTeammatePosition.load(IDB_TEAMMATE_POS);

  hCruise.load(IDB_CRUISE);
  hClimb.load(IDB_CLIMB);
  hFinalGlide.load(IDB_FINALGLIDE);
  hAbort.load(IDB_ABORT);

  // airspace brushes and colours

  hAirspaceBitmap[0].load(IDB_AIRSPACE0);
  hAirspaceBitmap[1].load(IDB_AIRSPACE1);
  hAirspaceBitmap[2].load(IDB_AIRSPACE2);
  hAirspaceBitmap[3].load(IDB_AIRSPACE3);
  hAirspaceBitmap[4].load(IDB_AIRSPACE4);
  hAirspaceBitmap[5].load(IDB_AIRSPACE5);
  hAirspaceBitmap[6].load(IDB_AIRSPACE6);
  hAirspaceBitmap[7].load(IDB_AIRSPACE7);

  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);

  for (i=0; i<NUMAIRSPACEBRUSHES; i++) {
    hAirspaceBrushes[i].set(hAirspaceBitmap[i]);
  }
  hAboveTerrainBrush.set(hAboveTerrainBitmap);

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

    hSnailColours[i] = Color((BYTE)Red, (BYTE)Green, (BYTE)Blue);
    hSnailPens[i].set(iwidth, hSnailColours[i]);

  }

  /* JMW created all re-used pens here */

  hpCompassBorder.set(IBLSCALE(3), Color(0xff,0xff,0xff));

  // testing only    Appearance.InverseAircraft = true;

  if (Appearance.InverseAircraft) {
    hpAircraft.set(IBLSCALE(3), Color(0x00,0x00,0x00));
    hpAircraftBorder.set(IBLSCALE(1), Color(0xff,0xff,0xff));
  } else {
    hpAircraft.set(IBLSCALE(3), Color(0xff,0xff,0xff));
    hpAircraftBorder.set(IBLSCALE(1), Color(0x00,0x00,0x00));
  }

#if (MONOCHROME_SCREEN > 0)
  hpWind.set(IBLSCALE(2), Color(0,0,0));
#else
  hpWind.set(IBLSCALE(2), Color(255,0,0));
#endif

  hpWindThick.set(IBLSCALE(4), Color(255,220,220));

  hpBearing.set(IBLSCALE(2), Color(0,0,0));
  hpBestCruiseTrack.set(IBLSCALE(1), Color(0,0,255));
#if (MONOCHROME_SCREEN > 0)
  hpCompass.set(IBLSCALE(1), Color(0x00,0x00,0x00));
  //hpCompass.set(1, Color(0xff,0xff,0xff));
#else
  hpCompass.set(IBLSCALE(1), Color(0xcf,0xcf,0xFF));
#endif
  hpThermalBand.set(IBLSCALE(2), Color(0x40,0x40,0xFF));
  hpThermalBandGlider.set(IBLSCALE(2), Color(0x00,0x00,0x30));

  hpFinalGlideBelow.set(IBLSCALE(1), Color(0xFF,0xA0,0xA0));
  hpFinalGlideBelowLandable.set(IBLSCALE(1), Color(255,196,0));

  // TODO enhancement: support red/green Color blind
  hpFinalGlideAbove.set(IBLSCALE(1), Color(0xA0,0xFF,0xA0));

  hpSpeedSlow.set(IBLSCALE(1), Color(0xFF,0x00,0x00));
  hpSpeedFast.set(IBLSCALE(1), Color(0x00,0xFF,0x00));

  hpStartFinishThick.set(IBLSCALE(5), TaskColor);

  hpStartFinishThin.set(IBLSCALE(1), Color(255,0,0));

  hpMapScale.set(IBLSCALE(1), Color(0,0,0));
  hpTerrainLine.set(Pen::DASH, 1, Color(0x30,0x30,0x30));
  hpTerrainLineBg.set(1, Color(0xFF,0xFF,0xFF));
  // VENTA3
  hpVisualGlideLightBlack.set(Pen::DASH, 1, Color(0x0,0x0,0x0));
  hpVisualGlideHeavyBlack.set(Pen::DASH, 2, Color(0x0,0x0,0x0));
  hpVisualGlideLightRed.set(Pen::DASH, 1, Color(0xff,0x0,0x0));
  hpVisualGlideHeavyRed.set(Pen::DASH, 2, Color(0xff,0x0,0x0));

#if (MONOCHROME_SCREEN > 0)
  hbCompass.set(Color(0xff,0xff,0xff));
#else
  hbCompass.set(Color(0x40,0x40,0xFF));
#endif
  hbThermalBand.set(Color(0x80,0x80,0xFF));
  hbBestCruiseTrack.set(Color(0x0,0x0,0xFF));
  hbFinalGlideBelow.set(Color(0xFF,0x00,0x00));
  hbFinalGlideBelowLandable.set(Color(0xFF,180,0x00));
  hbFinalGlideAbove.set(Color(0x00,0xFF,0x00));

#if (MONOCHROME_SCREEN > 0)
  hbWind.set(Color(0x80,0x80,0x80));
#else
  hbWind.set(Color(0x80,0x80,0x80));
#endif

  hBmpMapScale.load(IDB_MAPSCALE_A);

  hBrushFlyingModeAbort.set(Color(0xff,0x00,0x00));

  if (Appearance.IndLandable == wpLandableDefault){
    hBmpAirportReachable.load(IDB_REACHABLE);
    hBmpAirportUnReachable.load(IDB_LANDABLE);
    hBmpFieldReachable.load(IDB_REACHABLE);
    hBmpFieldUnReachable.load(IDB_LANDABLE);
  } else if (Appearance.IndLandable == wpLandableAltA){
    hBmpAirportReachable.load(IDB_AIRPORT_REACHABLE);
    hBmpAirportUnReachable.load(IDB_AIRPORT_UNREACHABLE);
    hBmpFieldReachable.load(IDB_OUTFILED_REACHABLE);
    hBmpFieldUnReachable.load(IDB_OUTFILED_UNREACHABLE);
  }

  hBmpThermalSource.load(IDB_THERMALSOURCE);
  hBmpTarget.load(IDB_TARGET);

  for (int i=0; i<AIRSPACECLASSCOUNT; i++) {
    hAirspacePens[i].set(IBLSCALE(2), GetAirspaceColourByClass(i));
  }

}

void ScreenGraphics::Destroy() {

  int i;

  hTurnPoint.reset();
  hSmall.reset();
  hCruise.reset();
  hClimb.reset();
  hFinalGlide.reset();
  hAutoMacCready.reset();
  hFLARMTraffic.reset();
  hTerrainWarning.reset();
  hGPSStatus1.reset();
  hGPSStatus2.reset();
  hAbort.reset();
  hLogger.reset();
  hLoggerOff.reset();

  hpAircraft.reset();
  hpAircraftBorder.reset();
  hpWind.reset();
  hpWindThick.reset();
  hpBearing.reset();
  hpBestCruiseTrack.reset();
  hpCompass.reset();
  hpThermalBand.reset();
  hpThermalBandGlider.reset();
  hpFinalGlideAbove.reset();
  hpFinalGlideBelow.reset();
  hpFinalGlideBelowLandable.reset();
  hpMapScale.reset();
  hpTerrainLine.reset();
  hpTerrainLineBg.reset();
  hpSpeedFast.reset();
  hpSpeedSlow.reset();
  hpStartFinishThick.reset();
  hpStartFinishThin.reset();

  hpVisualGlideLightBlack.reset(); // VENTA3
  hpVisualGlideLightRed.reset(); // VENTA3
  hpVisualGlideHeavyRed.reset(); // VENTA3
  hpVisualGlideHeavyBlack.reset(); // VENTA3

  hbCompass.reset();
  hbThermalBand.reset();
  hbBestCruiseTrack.reset();
  hbFinalGlideBelow.reset();
  hbFinalGlideBelowLandable.reset();
  hbFinalGlideAbove.reset();
  hbWind.reset();

  hBmpMapScale.reset();
  hBmpCompassBg.reset();
  hBackgroundBrush.reset();
  hBmpClimbeAbort.reset();

  hpCompassBorder.reset();
  hBrushFlyingModeAbort.reset();

  hBmpAirportReachable.reset();
  hBmpAirportUnReachable.reset();
  hBmpFieldReachable.reset();
  hBmpFieldUnReachable.reset();
  hBmpThermalSource.reset();
  hBmpTarget.reset();
  hBmpTeammatePosition.reset();

  for(i=0;i<NUMAIRSPACEBRUSHES;i++)
    {
      hAirspaceBrushes[i].reset();
      hAirspaceBitmap[i].reset();
    }

  hAboveTerrainBitmap.reset();
  hAboveTerrainBrush.reset();

  for (i=0; i<AIRSPACECLASSCOUNT; i++) {
    hAirspacePens[i].reset();
  }

  for (i=0; i<NUMSNAILCOLORS; i++) {
    hSnailPens[i].reset();
  }

  greenBrush.reset();
  yellowBrush.reset();
  redBrush.reset();

  infoSelectedBrush.reset();
  infoUnselectedBrush.reset();
  buttonBrush.reset();

  Units::UnLoadUnitBitmap();

}


void ClipDrawLine(Canvas &canvas, Pen::style PenStyle, const int width,
		  const POINT ptStart, const POINT ptEnd,
                  const Color cr, const RECT rc)
{
  int i;
  HPEN hpDash,hpOld;
  POINT pt[2];
  //Create a dot pen
  Pen pen(PenStyle, 1, cr);
  canvas.select(pen);

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
      canvas.clipped_polyline(pt, 2, rc);
    }
  } else {
    pt[0].y -= width / 2;
    pt[1].y -= width / 2;
    for (i = 0; i < width; i++){
      pt[0].y += 1;
      pt[1].y += 1;
      canvas.clipped_polyline(pt, 2, rc);
    }
  }
}


void DrawGreatCircle(Canvas &canvas,
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

  canvas.select(MapGfx.hpBearing);

  POINT StartP;
  POINT EndP;
  LatLon2Screen(startLon,
                startLat,
                StartP);
  LatLon2Screen(targetLon,
                targetLat,
                EndP);

  if (d_distance>distanceTotal) {
    canvas.clipped_line(StartP, EndP, rc);
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

      canvas.clipped_line(StartP, EndP, rc);

      StartP.x = EndP.x;
      StartP.y = EndP.y;

      startLat = tlat1;
      startLon = tlon1;

    }
  }
#else
  // Simple and this should work for PNA with display bug

  canvas.select(MapGfx.hpBearing);
  POINT pt[2];
  MapWindowProjection::LatLon2Screen(startLon,
                startLat,
                pt[0]);
  MapWindowProjection::LatLon2Screen(targetLon,
                targetLat,
                pt[1]);
  canvas.clipped_polygon(pt, 2, rc, false);

#endif
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
bool TextInBox(Canvas &canvas, const TCHAR* Value, int x, int y,
	       int size, TextInBoxMode_t Mode, bool noOverlap) {

  RECT brect;
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

  canvas.white_brush();

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
    canvas.select(Mode.AsFlag.Border ? MapWindowBoldFont : MapWindowFont);
  }

  SIZE tsize = canvas.text_size(Value);

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
        canvas.select(MapGfx.hpMapScale);
      } else {
        canvas.white_pen();
      }
      canvas.round_rectangle(brect.left, brect.top, brect.right, brect.bottom,
                             IBLSCALE(8), IBLSCALE(8));
#if (WINDOWSPC>0)
      canvas.background_transparent();
      canvas.text(x, y, Value);
#else
      canvas.text_opaque(x, y, NULL, Value);
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
      canvas.set_background_color(Color(0xff, 0xff, 0xff));
      canvas.text_opaque(x, y, &brect, Value);
      drawn=true;
    }

  } else if (Mode.AsFlag.WhiteBold) {

    brect.left = x-2;
    brect.right = brect.left+tsize.cx+4;
    brect.top = y+((tsize.cy+4)>>3)-2;
    brect.bottom = brect.top+3+tsize.cy-((tsize.cy+4)>>3);

    notoverlapping = checkLabelBlock(brect);

    if (!noOverlap || notoverlapping) {
      canvas.set_text_color(Color(0xff,0xff,0xff));

#if (WINDOWSPC>0)
      canvas.background_transparent();
      canvas.text(x+1, y, Value);
      canvas.text(x+2, y, Value);
      canvas.text(x-1, y, Value);
      canvas.text(x-2, y, Value);
      canvas.text(x, y+1, Value);
      canvas.text(x, y-1, Value);
      canvas.set_text_color(Color(0x00,0x00,0x00));

      canvas.text(x, y, Value);

#else
      canvas.text_opaque(x+2, y, NULL, Value);
      canvas.text_opaque(x+1, y, NULL, Value);
      canvas.text_opaque(x-1, y, NULL, Value);
      canvas.text_opaque(x-2, y, NULL, Value);
      canvas.text_opaque(x, y+1, NULL, Value);
      canvas.text_opaque(x, y-1, NULL, Value);
      canvas.set_text_color(Color(0x00,0x00,0x00));

      canvas.text_opaque(x, y, NULL, Value);
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
      canvas.background_transparent();
      canvas.text(x, y, Value);
#else
      canvas.text_opaque(x, y, NULL, Value);
#endif
      drawn=true;
    }

  }

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


