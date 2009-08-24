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

#ifndef SCREEN_GRAPHICS_HPP
#define SCREEN_GRAPHICS_HPP

#include "XCSoar.h"
#include "Airspace.h"

class ScreenGraphics {
public:
  void Initialise();
  void Destroy();

  HPEN hAirspacePens[AIRSPACECLASSCOUNT];
  HPEN hSnailPens[NUMSNAILCOLORS];
  COLORREF hSnailColours[NUMSNAILCOLORS];
  HBRUSH hAirspaceBrushes[NUMAIRSPACEBRUSHES];
  HBITMAP hAirspaceBitmap[NUMAIRSPACEBRUSHES];
  HBITMAP hAboveTerrainBitmap;
  HBRUSH hAboveTerrainBrush;
  COLORREF Colours[NUMAIRSPACECOLORS];

  HBITMAP
    hTurnPoint, hSmall, hCruise, hClimb,
    hFinalGlide, hAutoMacCready, hTerrainWarning, hGPSStatus1, hGPSStatus2,
    hAbort, hLogger, hLoggerOff, hFLARMTraffic;

  HBRUSH   hBackgroundBrush;

  COLORREF BackgroundColor;
  COLORREF TaskColor;

  HPEN hpAircraft;
  HPEN hpAircraftBorder;
  HPEN hpWind;
  HPEN hpWindThick;
  HPEN hpBearing;
  HPEN hpBestCruiseTrack;
  HPEN hpCompass;
  HPEN hpThermalBand;
  HPEN hpThermalBandGlider;
  HPEN hpFinalGlideAbove;
  HPEN hpFinalGlideBelow;
  HPEN hpFinalGlideBelowLandable;
  HPEN hpMapScale;
  HPEN hpTerrainLine;
  HPEN hpTerrainLineBg;
  HPEN hpVisualGlideLightRed; // VENTA3
  HPEN hpVisualGlideHeavyRed; //
  HPEN hpVisualGlideLightBlack; // VENTA3
  HPEN hpVisualGlideHeavyBlack; //
  HPEN hpVisualGlideExtra; // future use
  HPEN hpSpeedFast;
  HPEN hpSpeedSlow;
  HPEN hpStartFinishThick;
  HPEN hpStartFinishThin;

  HBRUSH hbCompass;
  HBRUSH hbThermalBand;
  HBRUSH hbBestCruiseTrack;
  HBRUSH hbFinalGlideBelow;
  HBRUSH hbFinalGlideBelowLandable;
  HBRUSH hbFinalGlideAbove;
  HBRUSH hbWind;

  HPEN    hpCompassBorder;
  HBRUSH  hBrushFlyingModeAbort;

  HBITMAP hBmpAirportReachable;
  HBITMAP hBmpAirportUnReachable;
  HBITMAP hBmpFieldReachable;
  HBITMAP hBmpFieldUnReachable;
  HBITMAP hBmpThermalSource;
  HBITMAP hBmpTarget;
  HBITMAP hBmpTeammatePosition;

  HBITMAP hBmpMapScale;
  HBITMAP hBmpCompassBg;
  HBITMAP hBmpClimbeAbort;
  HBITMAP hBmpUnitKm;
  HBITMAP hBmpUnitSm;
  HBITMAP hBmpUnitNm;
  HBITMAP hBmpUnitM;
  HBITMAP hBmpUnitFt;
  HBITMAP hBmpUnitMpS;
};

extern ScreenGraphics MapGfx;

#endif
