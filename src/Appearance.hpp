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

#ifndef APPEARANCE_H
#define APPEARANCE_H

typedef enum {
  wpLandableWinPilot = 0,
  wpLandableAltA,
  wpLandableAltB,
} IndLandable_t;

typedef enum {
  smAlignCenter = 0,
  smAlignTopLeft,
} StateMessageAlign_t;

typedef enum {
  /**
   * Use the platform default - i.e. keyboard if the device has a
   * pointing device.
   */
  tiDefault,
  tiKeyboard,
  tiHighScore,
} TextInputStyle_t;

typedef enum {
  dtText,
  dtIcon,
} DialogTabStyle_t;

typedef enum {
  apIbBox = 0,
  apIbTab
} InfoBoxBorderAppearance_t;

typedef enum {
  acSimple = 0,
  acDetailed,
} AircraftSymbol_t;

struct Appearance {
  IndLandable_t IndLandable;
  bool InverseInfoBox;
  StateMessageAlign_t StateMessageAlign;
  TextInputStyle_t TextInputStyle;
  DialogTabStyle_t DialogTabStyle;
  bool InfoBoxColors;
  InfoBoxBorderAppearance_t InfoBoxBorder;
  bool UseCustomFonts;
  bool UseSWLandablesRendering;
  int LandableRenderingScale;
  bool ScaleRunwayLength;
  AircraftSymbol_t AircraftSymbol;
};

extern struct Appearance Appearance;

#endif
