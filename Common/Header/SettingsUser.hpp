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
#ifndef XCSOAR_SETTINGS_USER_HPP
#define XCSOAR_SETTINGS_USER_HPP

// changed only in config or by user interface
// not expected to be used by other threads

#include "Navigation/GeoPoint.hpp"
#include "SettingsAirspace.hpp"

extern struct Appearance Appearance;

typedef enum {
  TRACKUP=0,
  NORTHUP=1,
  NORTHCIRCLE=2,
  TRACKCIRCLE=3,
  NORTHTRACK=4
} DisplayOrientation_t;

typedef enum {
  DISPLAYNAME=0,
  DISPLAYNUMBER=1,
  DISPLAYFIRSTFIVE=2,
  DISPLAYNONE=3,
  DISPLAYFIRSTTHREE=4,
  DISPLAYNAMEIFINTASK=5
} DisplayTextType_t;

typedef enum {
  dmNone,
  dmCircling,
  dmCruise,
  dmFinalGlide
} DisplayMode_t;

// user interface options

// where using these from Calculations or MapWindow thread, should
// protect

typedef struct SETTINGS_MAP_ {
  /** Map zooms in on circling */
  bool  CircleZoom;
  bool  ExtendedVisualGlide;
  /** Map will show topology */
  bool  EnableTopology;
  /** Map will show terrain */
  bool  EnableTerrain;
  unsigned char  DeclutterLabels;
  /** Snailtrail wind drifting in circling mode */
  bool  EnableTrailDrift;
  /** Show compass in cruise mode */
  bool  EnableCDICruise;
  /** Show compass in circling mode */
  bool  EnableCDICircling;
  /** Automatic zoom when closing in on waypoint */
  bool  AutoZoom;
  int   SnailWidthScale;
  int   WindArrowStyle;
  /** What type of text to draw next to the waypoint icon */
  DisplayTextType_t  DisplayTextType;
  int   TrailActive;
  int   VisualGlide;
  /** Airspaces are drawn with black border (otherwise in airspace color) */
  bool  bAirspaceBlackOutline;

  int   GliderScreenPosition;
  /** Orientation of the map (North up, Track up, etc.) */
  DisplayOrientation_t DisplayOrientation;

  /** Terrain contrast percentage */
  short TerrainContrast;
  /** Terrain brightness percentage */
  short TerrainBrightness;
  short TerrainRamp;
  int OnAirSpace; // VENTA3 toggle DrawAirSpace
  bool EnableAuxiliaryInfo;
  DisplayMode_t UserForceDisplayMode;
  /** Fullscreen mode (no infoboxes) */
  bool FullScreen;
  bool EnablePan;
  GEOPOINT PanLocation;
  bool   TargetPan;
  int    TargetPanIndex;
  double TargetZoomDistance;
  double MapScale;
  /** Show FLARM radar if traffic present */
  bool  EnableFLARMGauge;
  unsigned EnableFLARMMap;
  bool  ScreenBlanked;
  bool EnableAutoBlank;
  /** Show vario gauge */
  bool  EnableVarioGauge;
  /** Update system time from GPS time */
  bool SetSystemTimeFromGPS;

  int    iAirspaceBrush[AIRSPACECLASSCOUNT];
  int    iAirspaceColour[AIRSPACECLASSCOUNT];

} SETTINGS_MAP;

#endif
