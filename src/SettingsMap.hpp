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
#ifndef XCSOAR_SETTINGS_USER_HPP
#define XCSOAR_SETTINGS_USER_HPP

// changed only in config or by user interface
// not expected to be used by other threads

#include "Navigation/GeoPoint.hpp"
#include "SettingsAirspace.hpp"
#include "Screen/TextInBox.hpp"

enum DisplayOrientation_t {
  TRACKUP = 0,
  NORTHUP,
  TARGETUP
};

enum MapShiftBias_t {
  MAP_SHIFT_BIAS_NONE = 0,
  MAP_SHIFT_BIAS_TRACK,
  MAP_SHIFT_BIAS_TARGET
};

enum DisplayTextType_t {
  DISPLAYNAME = 0,
  OBSOLETE_DONT_USE_DISPLAYNUMBER,
  DISPLAYFIRSTFIVE,
  DISPLAYNONE,
  DISPLAYFIRSTTHREE,
  OBSOLETE_DONT_USE_DISPLAYNAMEIFINTASK,
  DISPLAYUNTILSPACE
};

enum WaypointArrivalHeightDisplay_t {
  WP_ARRIVAL_HEIGHT_NONE = 0,
  WP_ARRIVAL_HEIGHT_GLIDE,
  WP_ARRIVAL_HEIGHT_TERRAIN,
  WP_ARRIVAL_HEIGHT_GLIDE_AND_TERRAIN
};

enum WayPointLabelSelection_t {
  wlsAllWayPoints,
  wlsTaskAndLandableWayPoints,
  wlsTaskWayPoints,
  wlsNoWayPoints
};

enum DisplayMode_t{
  dmNone,
  dmCircling,
  dmCruise,
  dmFinalGlide
};

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

enum DisplayTrackBearing_t {
  dtbOff,
  dtbOn,
  dtbAuto
};


// user interface options

// where using these from Calculations or MapWindow thread, should
// protect

struct SETTINGS_MAP {
  /** Map zooms in on circling */
  bool CircleZoom;
  /** Maximum distance limit for AutoZoom */
  fixed MaxAutoZoomDistance;
  /** Map will show topography */
  bool EnableTopography;
  /** Map will show terrain */
  bool EnableTerrain;

  bool NorthArrow;

  /**
   * Apply slope shading to the terrain?
   */
  SlopeShadingType_t SlopeShadingType;

  /** Snailtrail wind drifting in circling mode */
  bool EnableTrailDrift;
  /** Indicate extra distance reqd. if deviating from target heading */
  bool EnableDetourCostMarker;
  /** Render track bearing on map */
  DisplayTrackBearing_t DisplayTrackBearing;

  /** Automatic zoom when closing in on waypoint */
  bool AutoZoom;
  bool SnailScaling;
  /** 0: standard, 1: seeyou colors */
  SnailType_t SnailType;
  int WindArrowStyle;
  /** What type of text to draw next to the waypoint icon */
  DisplayTextType_t DisplayTextType;
  /** Which arrival height to display next to waypoint labels */
  WaypointArrivalHeightDisplay_t WaypointArrivalHeightDisplay;
  /** What type of waypoint labels to render */
  WayPointLabelSelection_t WayPointLabelSelection;
  /** What type of waypoint labels to render */
  RenderMode LandableRenderMode;

  int TrailActive;
  /** Airspaces are drawn with black border (otherwise in airspace color) */
  bool bAirspaceBlackOutline;

#ifndef ENABLE_OPENGL
  /**
   * Should the airspace be rendered with a transparent brush instead
   * of a pattern brush?
   */
  bool airspace_transparency;

  /**
   * What portion of the airspace area should be filled with the
   * airspace brush?
   */
  enum AirspaceFillMode {
    /** the platform specific default is used */
    AS_FILL_DEFAULT,

    /** fill all of the area */
    AS_FILL_ALL,

    /** fill only a thick padding (like on ICAO maps) */
    AS_FILL_PADDING,
  } AirspaceFillMode;
#endif

  int GliderScreenPosition;
  /** Orientation of the map (North up, Track up, etc.) */
  DisplayOrientation_t OrientationCruise;
  DisplayOrientation_t OrientationCircling;

  /** The bias for map shifting (Heading, Target, etc.) */
  MapShiftBias_t MapShiftBias;

  /** Terrain contrast percentage */
  short TerrainContrast;
  /** Terrain brightness percentage */
  short TerrainBrightness;
  short TerrainRamp;
  bool EnableAirspace;
  bool EnableAuxiliaryInfo;
  unsigned AuxiliaryInfoBoxPanel;
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
  bool EnableFLARMMap;
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
