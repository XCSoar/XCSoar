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
#include "Screen/Point.hpp"
#include "Screen/Features.hpp"

class Bitmap;
class MaskedIcon;
class Brush;
struct Color;
class Pen;

class Canvas;
class Angle;

namespace Graphics {
  void Initialise();
  void Deinitialise();

#ifdef HAVE_HATCHED_BRUSH
  extern Bitmap hAboveTerrainBitmap;
  extern Brush hAboveTerrainBrush;
#endif

  extern MaskedIcon hTerrainWarning;
  extern MaskedIcon hLogger, hLoggerOff;
  extern MaskedIcon hCruise, hClimb, hFinalGlide, hAbort;
  extern MaskedIcon hGPSStatus1, hGPSStatus2;

  extern Pen hpWind, hpWindTail;
  extern Pen hpCompass;
  extern Pen hpTerrainLine;
  extern Pen hpTerrainLineThick;
  extern Pen hpTrackBearingLine;
  extern Pen ContestPen[3];

  extern Brush hbCompass;
  extern Brush hbWind;

  extern MaskedIcon hBmpThermalSource;
  extern MaskedIcon hBmpTrafficSafe;
  extern MaskedIcon hBmpTrafficWarning;
  extern MaskedIcon hBmpTrafficAlarm;

  extern MaskedIcon hBmpMapScaleLeft;
  extern MaskedIcon hBmpMapScaleRight;

  // task dialog
  extern Bitmap hBmpTabTask;
  extern Bitmap hBmpTabWrench;
  extern Bitmap hBmpTabSettings;
  extern Bitmap hBmpTabCalculator;

  // status dialog
  extern Bitmap hBmpTabFlight;
  extern Bitmap hBmpTabSystem;
  extern Bitmap hBmpTabRules;
  extern Bitmap hBmpTabTimes;

  extern Brush hbGround;

  extern const Color GroundColor;

  extern const Color skyColor;
  extern const Color seaColor;
};

#endif
