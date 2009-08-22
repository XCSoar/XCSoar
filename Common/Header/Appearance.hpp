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

#ifndef APPEARANCE_H
#define APPEARANCE_H

#include "StdAfx.h"
#include "Screen/Font.hpp"

typedef enum{
  apMsDefault=0,
  apMsNone,
  apMsAltA
}MapScaleAppearance_t;

typedef enum{
  apMs2Default=0,
  apMs2None,
  apMs2AltA
}MapScale2Appearance_t;

typedef enum{
  apFlightModeIconDefault=0,
  apFlightModeIconAltA
}FlightModeIconAppearance_t;

typedef enum{
  apCompassDefault=0,
  apCompassAltA
}CompassAppearance_t;

typedef enum{
  ctBestCruiseTrackDefault=0,
  ctBestCruiseTrackAltA,
}BestCruiseTrack_t;

typedef enum{
  afAircraftDefault=0,
  afAircraftAltA
}Aircraft_t;

typedef enum{
  fgFinalGlideDefault=0,
  fgFinalGlideAltA,
}IndFinalGlide_t;

typedef enum{
  wpLandableDefault=0,
  wpLandableAltA,
}IndLandable_t;

typedef enum{
  smAlligneCenter=0,
  smAlligneTopLeft,
}StateMessageAlligne_t;

typedef enum{
  tiHighScore=0,
  tiKeyboard,
}TextInputStyle_t;

typedef enum{
  gvnsDefault=0,
  gvnsLongNeedle,
}GaugeVarioNeedleStyle_t;

typedef enum{
  apIbBox=0,
  apIbTab
}InfoBoxBorderAppearance_t;

typedef enum{
  apIg0=0,
  apIg1,
  apIg2,
  apIg3,
  apIg4,
  apIg5,
  apIg6,
  apIg7
}InfoBoxGeomAppearance_t;

typedef struct{
  MapScaleAppearance_t MapScale;
  MapScale2Appearance_t MapScale2;
  bool DontShowLoggerIndicator;
  int DefaultMapWidth;
  POINT GPSStatusOffset;
  FlightModeIconAppearance_t FlightModeIcon;
  POINT FlightModeOffset;
  CompassAppearance_t CompassAppearance;
  FontHeightInfo_t TitleWindowFont;
  FontHeightInfo_t MapWindowFont;
  FontHeightInfo_t MapWindowBoldFont;
  FontHeightInfo_t InfoWindowFont;
  FontHeightInfo_t CDIWindowFont;
  FontHeightInfo_t StatisticsFont;
  FontHeightInfo_t MapLabelFont; // VENTA6 added
  FontHeightInfo_t TitleSmallWindowFont;
  BestCruiseTrack_t BestCruiseTrack;
  Aircraft_t Aircraft;
  bool DontShowSpeedToFly;
  IndFinalGlide_t IndFinalGlide;
  IndLandable_t IndLandable;
  bool DontShowAutoMacCready;
  bool InverseInfoBox;
  bool InfoTitelCapital;
  StateMessageAlligne_t StateMessageAlligne;
  TextInputStyle_t TextInputStyle;
  bool GaugeVarioAvgText;
  bool GaugeVarioMc;
  bool GaugeVarioSpeedToFly;
  bool GaugeVarioBallast;
  bool GaugeVarioBugs;
  GaugeVarioNeedleStyle_t GaugeVarioNeedleStyle;
  bool InfoBoxColors;
  InfoBoxBorderAppearance_t InfoBoxBorder;
#if defined(PNA) || defined(FIVV)
  InfoBoxGeomAppearance_t InfoBoxGeom; // VENTA-ADDON
  InfoBoxModelAppearance_t InfoBoxModel; // VENTA-ADDON model change
#endif
  bool InverseAircraft;
  bool GaugeVarioGross;
  bool GaugeVarioAveNeedle;
} Appearance_t;

#endif
