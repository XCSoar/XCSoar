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
#include "Screen/Ramp.hpp"
#include "Appearance.hpp"
#include "MapWindow.h"
#include "Interface.hpp"
#include "InfoBoxLayout.h"

#define NUMSNAILRAMP 6

const COLORRAMP snail_colors[] = {
  {0,         0xff, 0x3e, 0x00},
  {50,        0xcd, 0x4f, 0x27},
  {100,       0x8f, 0x8f, 0x8f},
  {150,       0x27, 0xcd, 0x4f},
  {201,       0x00, 0xff, 0x3e},
  {501,       0x00, 0xff, 0x3e}
};

void ScreenGraphics::Initialise(void) {
  int i;

  TaskColor = RGB(0,120,0); // was 255

  Colours[ 0]= RGB(0xFF,0x00,0x00);
  Colours[ 1]= RGB(0x00,0xFF,0x00);
  Colours[ 2]= RGB(0x00,0x00,0xFF);
  Colours[ 3]= RGB(0xFF,0xFF,0x00);
  Colours[ 4]= RGB(0xFF,0x00,0xFF);
  Colours[ 5]= RGB(0x00,0xFF,0xFF);
  Colours[ 6]= RGB(0x7F,0x00,0x00);
  Colours[ 7]= RGB(0x00,0x7F,0x00);
  Colours[ 8]= RGB(0x00,0x00,0x7F);
  Colours[ 9]= RGB(0x7F,0x7F,0x00);
  Colours[10]= RGB(0x7F,0x00,0x7F);
  Colours[11]= RGB(0x00,0x7F,0x7F);
  Colours[12]= RGB(0xFF,0xFF,0xFF);
  Colours[13]= RGB(0xC0,0xC0,0xC0);
  Colours[14]= RGB(0x7F,0x7F,0x7F);
  Colours[15]= RGB(0x00,0x00,0x00);

  BackgroundColor = RGB(0xFF,0xFF,0xFF);

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
  minwidth = max(IBLSCALE(2),IBLSCALE(MapWindow::SnailWidthScale)/16);
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
		   *IBLSCALE(MapWindow::SnailWidthScale)/NUMSNAILCOLORS);
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
      CreatePen(PS_SOLID, IBLSCALE(2), Colours[MapWindow::iAirspaceColour[i]]);
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

}
