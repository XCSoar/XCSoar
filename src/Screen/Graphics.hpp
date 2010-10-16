/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Sizes.h"
#include "Screen/Bitmap.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Airspace/AirspaceClass.hpp"

struct SETTINGS_MAP;
class LabelBlock;

namespace Graphics {
  void Initialise();
  void InitialiseConfigured(const SETTINGS_MAP &settings_map);
  void InitSnailTrail(const SETTINGS_MAP &settings_map);
  void InitLandableIcons();
  void InitAirspacePens(const SETTINGS_MAP &settings_map);

  // airspace brushes/colours
  const Color GetAirspaceColour(const int i);
  const Brush &GetAirspaceBrush(const int i);
  const Color GetAirspaceColourByClass(const int i, const SETTINGS_MAP &settings);
  const Brush &GetAirspaceBrushByClass(const int i, const SETTINGS_MAP &settings);

  extern Pen hAirspacePens[AIRSPACECLASSCOUNT];
  extern Brush hAirspaceBrushes[NUMAIRSPACEBRUSHES];
  extern Bitmap hAirspaceBitmap[NUMAIRSPACEBRUSHES];

  extern Brush infoSelectedBrush;
  extern Brush infoUnselectedBrush;

  extern Pen hSnailPens[NUMSNAILCOLORS];
  extern Color hSnailColours[NUMSNAILCOLORS];

  extern Bitmap hAboveTerrainBitmap;
  extern Brush hAboveTerrainBrush;
  extern MaskedIcon hAirspaceInterceptBitmap;
  extern MaskedIcon hTerrainWarning;
  extern MaskedIcon hFLARMTraffic;
  extern MaskedIcon hLogger, hLoggerOff;
  extern MaskedIcon hCruise, hClimb, hFinalGlide, hAbort;
  extern MaskedIcon hAutoMacCready;
  extern MaskedIcon hGPSStatus1, hGPSStatus2;

  extern Brush hBackgroundBrush;

  extern Pen hpAircraft;
  extern Pen hpAircraftBorder;
  extern Pen hpWind;
  extern Pen hpWindThick;
  extern Pen hpBearing;
  extern Pen hpBestCruiseTrack;
  extern Pen hpCompass;
  extern Pen hpThermalBand;
  extern Pen hpThermalBandGlider;
  extern Pen hpFinalGlideAbove;
  extern Pen hpFinalGlideBelow;
  extern Pen hpFinalGlideBelowLandable;
  extern Pen hpMapScale;
  extern Pen hpTerrainLine;
  extern Pen hpTerrainLineBg;
  extern Pen hpSpeedFast;
  extern Pen hpSpeedSlow;
  extern Pen hpStartFinishThick;
  extern Pen hpStartFinishThin;

  extern Brush hbCompass;
  extern Brush hbThermalBand;
  extern Brush hbBestCruiseTrack;
  extern Brush hbFinalGlideBelow;
  extern Brush hbFinalGlideBelowLandable;
  extern Brush hbFinalGlideAbove;
  extern Brush hbWind;

  extern Pen hpCompassBorder;
  extern Brush hBrushFlyingModeAbort;

  extern MaskedIcon SmallIcon, TurnPointIcon;
  extern MaskedIcon AirportReachableIcon, AirportUnreachableIcon;
  extern MaskedIcon FieldReachableIcon, FieldUnreachableIcon;
  extern MaskedIcon hBmpThermalSource;
  extern MaskedIcon hBmpTarget;
  extern MaskedIcon hBmpTeammatePosition;

  extern Bitmap hBmpMapScale;
  extern Bitmap hBmpClimbeAbort;
  extern Bitmap hBmpUnitKm;
  extern Bitmap hBmpUnitSm;
  extern Bitmap hBmpUnitNm;
  extern Bitmap hBmpUnitM;
  extern Bitmap hBmpUnitFt;
  extern Bitmap hBmpUnitMpS;

  // used for flarm
  extern Brush AlarmBrush;
  extern Brush WarningBrush;
  extern Brush TrafficBrush;

  // misc
  extern const Color BackgroundColor;
  extern const Color TaskColor;
  extern const Color Colours[NUMAIRSPACECOLORS];

  // used by infobox and gauges
  extern const Color inv_redColor;
  extern const Color inv_blueColor;
  // Not really used, tested and dropped. But useful for the future
  extern const Color inv_yellowColor;
  extern const Color inv_greenColor;
  extern const Color inv_magentaColor;

  // these used for infoboxes/buttons
  extern const Color ColorSelected;
  extern const Color ColorUnselected;
  extern const Color ColorWarning;
  extern const Color ColorOK;
  extern const Color ColorBlack;
  extern const Color ColorMidGrey;
};

#endif
