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
#include "Screen/Canvas.hpp"
#include "Airspace/AirspaceClass.hpp"

struct SETTINGS_MAP;
class LabelBlock;

typedef union
{
  unsigned int AsInt;
  struct
  {
    unsigned Border :1;
    unsigned FillBackground :1;
    unsigned AlignRight :1;
    unsigned Reachable :1;
    unsigned AlignCenter :1;
    unsigned WhiteBorder :1;
    unsigned WhiteBold :1;
    unsigned NoSetFont :1;
    unsigned Color :3;
  } AsFlag;
} TextInBoxMode_t;

// mode are flags
// bit 0 == fill background add border / 1
// bit 1 == fill background            / 2
// bit 2 == right alligned             / 4
// bit 3 == landable TP label          / 8
// bit 4 == center alligned

class ScreenGraphics {
public:
  void Initialise(const SETTINGS_MAP &settings_map);
  ~ScreenGraphics();

  // airspace brushes/colours
  const Color GetAirspaceColour(const int i);
  const Brush &GetAirspaceBrush(const int i);
  const Color GetAirspaceColourByClass(const int i, const SETTINGS_MAP &settings);
  const Brush &GetAirspaceBrushByClass(const int i, const SETTINGS_MAP &settings);

  Pen hAirspacePens[AIRSPACECLASSCOUNT];
  Brush hAirspaceBrushes[NUMAIRSPACEBRUSHES];
  Bitmap hAirspaceBitmap[NUMAIRSPACEBRUSHES];

  Brush infoSelectedBrush;
  Brush infoUnselectedBrush;

  Pen hSnailPens[NUMSNAILCOLORS];
  Color hSnailColours[NUMSNAILCOLORS];

  Bitmap hAboveTerrainBitmap;
  Brush hAboveTerrainBrush;
  MaskedIcon hAirspaceInterceptBitmap;
  MaskedIcon hTerrainWarning;
  MaskedIcon hFLARMTraffic;
  MaskedIcon hLogger, hLoggerOff;
  MaskedIcon hCruise, hClimb, hFinalGlide, hAbort;
  MaskedIcon hAutoMacCready;
  MaskedIcon hGPSStatus1, hGPSStatus2;

  Brush hBackgroundBrush;

  Pen hpAircraft;
  Pen hpAircraftBorder;
  Pen hpWind;
  Pen hpWindThick;
  Pen hpBearing;
  Pen hpBestCruiseTrack;
  Pen hpCompass;
  Pen hpThermalBand;
  Pen hpThermalBandGlider;
  Pen hpFinalGlideAbove;
  Pen hpFinalGlideBelow;
  Pen hpFinalGlideBelowLandable;
  Pen hpMapScale;
  Pen hpTerrainLine;
  Pen hpTerrainLineBg;
  Pen hpVisualGlideLightRed;
  Pen hpVisualGlideHeavyRed;
  Pen hpVisualGlideLightBlack;
  Pen hpVisualGlideHeavyBlack;
  Pen hpVisualGlideExtra;
  Pen hpSpeedFast;
  Pen hpSpeedSlow;
  Pen hpStartFinishThick;
  Pen hpStartFinishThin;

  Brush hbCompass;
  Brush hbThermalBand;
  Brush hbBestCruiseTrack;
  Brush hbFinalGlideBelow;
  Brush hbFinalGlideBelowLandable;
  Brush hbFinalGlideAbove;
  Brush hbWind;

  Pen hpCompassBorder;
  Brush hBrushFlyingModeAbort;

  MaskedIcon SmallIcon, TurnPointIcon;
  MaskedIcon AirportReachableIcon, AirportUnreachableIcon;
  MaskedIcon FieldReachableIcon, FieldUnreachableIcon;
  MaskedIcon hBmpThermalSource;
  MaskedIcon hBmpTarget;
  MaskedIcon hBmpTeammatePosition;

  Bitmap hBmpMapScale;
  Bitmap hBmpClimbeAbort;
  Bitmap hBmpUnitKm;
  Bitmap hBmpUnitSm;
  Bitmap hBmpUnitNm;
  Bitmap hBmpUnitM;
  Bitmap hBmpUnitFt;
  Bitmap hBmpUnitMpS;

  // used for flarm
  Brush AlarmBrush;
  Brush WarningBrush;
  Brush TrafficBrush;

  // misc
  static const Color BackgroundColor;
  static const Color TaskColor;
  static const Color Colours[NUMAIRSPACECOLORS];

  // used by infobox and gauges
  static const Color inv_redColor;
  static const Color inv_blueColor;
  // Not really used, tested and dropped. But useful for the future
  static const Color inv_yellowColor;
  static const Color inv_greenColor;
  static const Color inv_magentaColor;

  // these used for infoboxes/buttons
  static const Color ColorSelected;
  static const Color ColorUnselected;
  static const Color ColorWarning;
  static const Color ColorOK;
  static const Color ColorBlack;
  static const Color ColorMidGrey;
};

extern ScreenGraphics MapGfx;

bool TextInBox(Canvas &canvas, const TCHAR *Value, int x, int y,
    TextInBoxMode_t Mode, const RECT MapRect, LabelBlock *label_block = NULL);

#endif
