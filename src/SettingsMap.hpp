/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef XCSOAR_SETTINGS_USER_HPP
#define XCSOAR_SETTINGS_USER_HPP

// changed only in config or by user interface
// not expected to be used by other threads

#include "Navigation/GeoPoint.hpp"
#include "SettingsAirspace.hpp"

enum DisplayOrientation_t {
  TRACKUP = 0,
  NORTHUP,
  TARGETUP
};

typedef enum {
  DISPLAYNAME = 0,
  DISPLAYNUMBER,
  DISPLAYFIRSTFIVE,
  DISPLAYNONE,
  DISPLAYFIRSTTHREE,
  OBSOLETE_DONT_USE_DISPLAYNAMEIFINTASK,
  DISPLAYUNTILSPACE
} DisplayTextType_t;

typedef enum {
  wlsAllWayPoints,
  wlsTaskAndLandableWayPoints,
  wlsTaskWayPoints,
  wlsNoWayPoints
} WayPointLabelSelection_t;

typedef enum {
  dmNone,
  dmCircling,
  dmCruise,
  dmFinalGlide
} DisplayMode_t;

enum SnailType_t {
  stStandardVario,
  stSeeYouVario,
  stAltitude,
};

enum SlopeShadingType_t {
  sstOff,
  sstFixed,
  sstSun,
  sstWind,
};

// user interface options

// where using these from Calculations or MapWindow thread, should
// protect

struct SETTINGS_MAP {
  /** Map zooms in on circling */
  bool CircleZoom;
  /** Map will show topology */
  bool EnableTopology;
  /** Map will show terrain */
  bool EnableTerrain;

  bool NorthArrow;

  /**
   * Apply slope shading to the terrain?
   */
  SlopeShadingType_t SlopeShadingType;

  /**
   * 0: show all labels
   * 1: show decluttered labels
   * 2: show no labels
   */
  unsigned char DeclutterLabels;
  /** Snailtrail wind drifting in circling mode */
  bool EnableTrailDrift;
  /** Indicate extra distance reqd. if deviating from target heading */
  bool EnableDetourCostMarker;

  /** Automatic zoom when closing in on waypoint */
  bool AutoZoom;
  bool SnailScaling;
  /** 0: standard, 1: seeyou colors */
  SnailType_t SnailType;
  int WindArrowStyle;
  /** What type of text to draw next to the waypoint icon */
  DisplayTextType_t DisplayTextType;
  /** What type of waypoint labels to render */
  WayPointLabelSelection_t WayPointLabelSelection;

  int TrailActive;
  /** Airspaces are drawn with black border (otherwise in airspace color) */
  bool bAirspaceBlackOutline;

  int GliderScreenPosition;
  /** Orientation of the map (North up, Track up, etc.) */
  DisplayOrientation_t OrientationCruise;
  DisplayOrientation_t OrientationCircling;

  /** Terrain contrast percentage */
  short TerrainContrast;
  /** Terrain brightness percentage */
  short TerrainBrightness;
  short TerrainRamp;
  bool EnableAirspace;
  bool EnableAuxiliaryInfo;
  DisplayMode_t UserForceDisplayMode;
  bool EnablePan;
  GeoPoint PanLocation;
  bool TargetPan;
  unsigned TargetPanIndex;
  fixed TargetZoomDistance;
  /** Show FLARM radar if traffic present */
  bool EnableFLARMGauge;
  /** Automatically close the FLARM dialog when no traffic present */
  bool AutoCloseFlarmDialog;
  /** Show ThermalAssistant if circling */
  bool EnableTAGauge;
  unsigned EnableFLARMMap;
  bool ScreenBlanked;
  bool EnableAutoBlank;
  /** Show vario gauge */
  bool  EnableVarioGauge;
  /** Update system time from GPS time */
  bool SetSystemTimeFromGPS;

  int iAirspaceBrush[AIRSPACECLASSCOUNT];
  int iAirspaceColour[AIRSPACECLASSCOUNT];
};

#endif
