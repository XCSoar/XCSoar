/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Airspace/AirspaceClass.hpp"

class Bitmap;
class MaskedIcon;
class Brush;
struct Color;
class Pen;

struct SETTINGS_MAP;
class LabelBlock;

class Canvas;
class Angle;
#include "Screen/Point.hpp"

namespace Graphics {
  void Initialise();
  void InitialiseConfigured(const SETTINGS_MAP &settings_map);
  void InitSnailTrail(const SETTINGS_MAP &settings_map);
  void InitLandableIcons();
  void InitAirspacePens(const SETTINGS_MAP &settings_map);
  void Deinitialise();
  void DrawAircraft(Canvas &canvas, const Angle angle,
                    const RasterPoint aircraft_pos);

  // airspace brushes/colours
  const Color GetAirspaceColour(const int i);

#ifndef ENABLE_SDL
  const Brush &GetAirspaceBrush(const int i);
#endif

  const Color GetAirspaceColourByClass(const int i, const SETTINGS_MAP &settings);

#ifndef ENABLE_SDL
  const Brush &GetAirspaceBrushByClass(const int i, const SETTINGS_MAP &settings);
#endif

  extern Pen hAirspacePens[AIRSPACECLASSCOUNT];
#ifndef ENABLE_SDL
  extern Brush hAirspaceBrushes[NUMAIRSPACEBRUSHES];
  extern Bitmap hAirspaceBitmap[NUMAIRSPACEBRUSHES];
#endif

  extern Pen hpSnailVario[NUMSNAILCOLORS];

#ifndef ENABLE_SDL
  extern Bitmap hAboveTerrainBitmap;
  extern Brush hAboveTerrainBrush;
#endif

  extern MaskedIcon hAirspaceInterceptBitmap;
  extern MaskedIcon hTerrainWarning;
  extern MaskedIcon hFLARMTraffic;
  extern MaskedIcon hLogger, hLoggerOff;
  extern MaskedIcon hCruise, hClimb, hFinalGlide, hAbort;
  extern MaskedIcon hGPSStatus1, hGPSStatus2;

  extern Brush hBackgroundBrush;

  extern Pen hpAircraft;
  extern Pen hpAircraftBorder;
  extern Pen hpWind;
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

  extern Brush hbCompass;
  extern Brush hbThermalBand;
  extern Brush hbBestCruiseTrack;
  extern Brush hbFinalGlideBelow;
  extern Brush hbFinalGlideBelowLandable;
  extern Brush hbFinalGlideAbove;
  extern Brush hbWind;

  extern MaskedIcon SmallIcon, TurnPointIcon;
  extern MaskedIcon AirportReachableIcon, AirportUnreachableIcon;
  extern MaskedIcon FieldReachableIcon, FieldUnreachableIcon;
  extern MaskedIcon hBmpThermalSource;
  extern MaskedIcon hBmpTarget;
  extern MaskedIcon hBmpTeammatePosition;

  extern MaskedIcon hBmpMapScaleLeft;
  extern MaskedIcon hBmpMapScaleRight;

  // used for flarm
  extern Brush AlarmBrush;
  extern Brush WarningBrush;
  extern Brush TrafficBrush;

  // misc
  extern const Color TaskColor;
  extern const Color Colours[NUMAIRSPACECOLORS];

  // used by infobox and gauges
  extern const Color inv_redColor;
  extern const Color inv_blueColor;
  // Not really used, tested and dropped. But useful for the future
  extern const Color inv_yellowColor;
  extern const Color inv_greenColor;
  extern const Color inv_magentaColor;
};

#endif
