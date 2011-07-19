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
#include "Screen/Graphics.hpp"
#include "Screen/Point.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Canvas.hpp"
#include "Math/Screen.hpp"
#include "Appearance.hpp"
#include "SettingsMap.hpp"
#include "resource.h"
#include "Asset.hpp"
#include "LogFile.hpp"

Pen Graphics::hpSnail[NUMSNAILCOLORS];
Pen Graphics::hpSnailVario[NUMSNAILCOLORS];

#ifdef HAVE_HATCHED_BRUSH
Bitmap Graphics::hAboveTerrainBitmap;
Brush Graphics::hAboveTerrainBrush;
#endif

MaskedIcon Graphics::hTerrainWarning;
MaskedIcon Graphics::hLogger, Graphics::hLoggerOff;
MaskedIcon Graphics::hCruise, Graphics::hClimb,
           Graphics::hFinalGlide, Graphics::hAbort;
MaskedIcon Graphics::hGPSStatus1, Graphics::hGPSStatus2;
MaskedIcon Graphics::hBmpTrafficSafe, Graphics::hBmpTrafficWarning, Graphics::hBmpTrafficAlarm;

Pen Graphics::hpAircraft;
Pen Graphics::hpAircraftSimple1;
Pen Graphics::hpAircraftSimple2;
Pen Graphics::hpCanopy;

Pen Graphics::hpWind;
Pen Graphics::hpCompass;
Pen Graphics::hpFinalGlideAbove;
Pen Graphics::hpFinalGlideBelow;
Pen Graphics::hpFinalGlideBelowLandable;
Pen Graphics::hpMapScale;
Pen Graphics::hpTerrainLine;
Pen Graphics::hpTerrainLineThick;
Pen Graphics::hpTrackBearingLine;
Pen Graphics::TracePen;
Pen Graphics::ContestPen[3];

Brush Graphics::hbCanopy;
Brush Graphics::hbCompass;
Brush Graphics::hbFinalGlideBelow;
Brush Graphics::hbFinalGlideBelowLandable;
Brush Graphics::hbFinalGlideAbove;
Brush Graphics::hbWind;

MaskedIcon Graphics::SmallIcon, Graphics::TurnPointIcon, Graphics::TaskTurnPointIcon;
MaskedIcon Graphics::MountainTopIcon, Graphics::BridgeIcon, Graphics::TunnelIcon;
MaskedIcon Graphics::TowerIcon, Graphics::PowerPlantIcon;
MaskedIcon Graphics::AirportReachableIcon, Graphics::AirportUnreachableIcon;
MaskedIcon Graphics::AirportMarginalIcon, Graphics::FieldMarginalIcon;
MaskedIcon Graphics::FieldReachableIcon, Graphics::FieldUnreachableIcon;
MaskedIcon Graphics::hBmpThermalSource;

MaskedIcon Graphics::hBmpMapScaleLeft;
MaskedIcon Graphics::hBmpMapScaleRight;

Bitmap Graphics::hBmpTabTask;
Bitmap Graphics::hBmpTabWrench;
Bitmap Graphics::hBmpTabSettings;
Bitmap Graphics::hBmpTabCalculator;

Bitmap Graphics::hBmpTabFlight;
Bitmap Graphics::hBmpTabSystem;
Bitmap Graphics::hBmpTabRules;
Bitmap Graphics::hBmpTabTimes;

// used for landable rendering
Brush Graphics::hbGreen;
Brush Graphics::hbWhite;
Brush Graphics::hbMagenta;
Brush Graphics::hbOrange;
Brush Graphics::hbRed;
Brush Graphics::hbLightGray;
Brush Graphics::hbNotReachableTerrain;
Brush Graphics::hbGround;

static Color clrSepia(0x78,0x31,0x18);
const Color Graphics::GroundColor = Color(0x80,0x45,0x15);
const Color Graphics::skyColor = Color(0x0a,0xb9,0xf3);
const Color Graphics::seaColor = Color(0xbd,0xc5,0xd5); // ICAO open water area

void
Graphics::Initialise()
{
  /// @todo enhancement: support red/green color blind pilots with adjusted colour scheme

  LogStartUp(_T("Initialise graphics"));

  LoadUnitSymbols();

  hTerrainWarning.load_big(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);
  hGPSStatus1.load_big(IDB_GPSSTATUS1, IDB_GPSSTATUS1_HD, false);
  hGPSStatus2.load_big(IDB_GPSSTATUS2, IDB_GPSSTATUS2_HD, false);
  hLogger.load_big(IDB_LOGGER, IDB_LOGGER_HD);
  hLoggerOff.load_big(IDB_LOGGEROFF, IDB_LOGGEROFF_HD);

  hCruise.load_big(IDB_CRUISE, IDB_CRUISE_HD, false);
  hClimb.load_big(IDB_CLIMB, IDB_CLIMB_HD, false);
  hFinalGlide.load_big(IDB_FINALGLIDE, IDB_FINALGLIDE_HD, false);
  hAbort.load_big(IDB_ABORT, IDB_ABORT_HD, false);

#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);

  hAboveTerrainBrush.set(hAboveTerrainBitmap);
#endif

  hpWind.set(Layout::Scale(1), dark_color(COLOR_GRAY));
  hbWind.set(COLOR_GRAY);

  hBmpMapScaleLeft.load_big(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  hBmpMapScaleRight.load_big(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);

  hBmpTabTask.load((Layout::scale > 1) ? IDB_TASK_HD : IDB_TASK);
  hBmpTabWrench.load((Layout::scale > 1) ? IDB_WRENCH_HD : IDB_WRENCH);
  hBmpTabSettings.load((Layout::scale > 1) ? IDB_SETTINGS_HD : IDB_SETTINGS);
  hBmpTabCalculator.load((Layout::scale > 1) ? IDB_CALCULATOR_HD : IDB_CALCULATOR);

  hBmpTabFlight.load((Layout::scale > 1) ? IDB_GLOBE_HD : IDB_GLOBE);
  hBmpTabSystem.load((Layout::scale > 1) ? IDB_DEVICE_HD : IDB_DEVICE);
  hBmpTabRules.load((Layout::scale > 1) ? IDB_RULES_HD : IDB_RULES);
  hBmpTabTimes.load((Layout::scale > 1) ? IDB_CLOCK_HD : IDB_CLOCK);

  hBmpThermalSource.load_big(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);

  hBmpTrafficSafe.load_big(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  hBmpTrafficWarning.load_big(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  hBmpTrafficAlarm.load_big(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  hbCompass.set(Color(207, 207, 207));

  hbFinalGlideBelow.set(COLOR_RED);
  hpFinalGlideBelow.set(Layout::Scale(1), dark_color(COLOR_RED));

  hbFinalGlideBelowLandable.set(COLOR_ORANGE);
  hpFinalGlideBelowLandable.set(Layout::Scale(1), dark_color(COLOR_ORANGE));

  hbFinalGlideAbove.set(COLOR_GREEN);
  hpFinalGlideAbove.set(Layout::Scale(1), dark_color(COLOR_GREEN));

  hpCompass.set(Layout::Scale(1), COLOR_GRAY);

  hpMapScale.set(Layout::Scale(1), COLOR_BLACK);

  hpTerrainLine.set(Pen::DASH, Layout::Scale(1), clrSepia);
  hpTerrainLineThick.set(Pen::DASH, Layout::Scale(2), clrSepia);

  TracePen.set(2, Color(50, 243, 45));
  ContestPen[0].set(Layout::Scale(1)+2, COLOR_RED);
  ContestPen[1].set(Layout::Scale(1)+1, COLOR_ORANGE);
  ContestPen[2].set(Layout::Scale(1), COLOR_BLUE);

  SmallIcon.load_big(IDB_SMALL, IDB_SMALL_HD);
  TurnPointIcon.load_big(IDB_TURNPOINT, IDB_TURNPOINT_HD);
  TaskTurnPointIcon.load_big(IDB_TASKTURNPOINT, IDB_TASKTURNPOINT_HD);
  MountainTopIcon.load_big(IDB_MOUNTAIN_TOP, IDB_MOUNTAIN_TOP_HD);
  BridgeIcon.load_big(IDB_BRIDGE, IDB_BRIDGE_HD);
  TunnelIcon.load_big(IDB_TUNNEL, IDB_TUNNEL_HD);
  TowerIcon.load_big(IDB_TOWER, IDB_TOWER_HD);
  PowerPlantIcon.load_big(IDB_POWER_PLANT, IDB_POWER_PLANT_HD);

  hpAircraft.set(1, COLOR_DARK_GRAY);
  hpAircraftSimple1.set(Layout::Scale(1), COLOR_BLACK);
  hpAircraftSimple2.set(Layout::Scale(3), COLOR_WHITE);
  hpCanopy.set(1, dark_color(COLOR_CYAN));
  hbCanopy.set(COLOR_CYAN);

    // used for landable rendering
  hbGreen.set(COLOR_GREEN);
  hbWhite.set(COLOR_WHITE);
  hbMagenta.set(COLOR_MAGENTA);
  hbOrange.set(COLOR_ORANGE);
  hbRed.set(COLOR_RED);
  hbLightGray.set(COLOR_LIGHT_GRAY);
  hbNotReachableTerrain.set(light_color(COLOR_RED));

  hbGround.set(GroundColor);

  hpTrackBearingLine.set(3, COLOR_GRAY);
}

void
Graphics::InitialiseConfigured(const SETTINGS_MAP &settings_map)
{
  InitSnailTrail(settings_map);
  InitLandableIcons(settings_map.waypoint);
}

void
Graphics::InitSnailTrail(const SETTINGS_MAP &settings_map)
{
  const ColorRamp snail_colors_vario[] = {
    {0,   0xc4, 0x80, 0x1e}, // sinkColor
    {100, 0xa0, 0xa0, 0xa0},
    {200, 0x1e, 0xf1, 0x73} // liftColor
  };

  const ColorRamp snail_colors_vario2[] = {
    {0,   0x00, 0x00, 0xff},
    {99,  0x00, 0xff, 0xff},
    {100, 0xff, 0xff, 0x00},
    {200, 0xff, 0x00, 0x00}
  };

  const ColorRamp snail_colors_alt[] = {
    {0,   0xff, 0x00, 0x00},
    {50,  0xff, 0xff, 0x00},
    {100, 0x00, 0xff, 0x00},
    {150, 0x00, 0xff, 0xff},
    {200, 0x00, 0x00, 0xff},
  };

  int iwidth;
  int minwidth = Layout::Scale(2);

  for (int i = 0; i < NUMSNAILCOLORS; i++) {
    short ih = i * 200 / (NUMSNAILCOLORS - 1);
    Color color = (settings_map.SnailType == stAltitude) ?
                  ColorRampLookup(ih, snail_colors_alt, 5) :
                  (settings_map.SnailType == stSeeYouVario) ?
                  ColorRampLookup(ih, snail_colors_vario2, 4) :
                  ColorRampLookup(ih, snail_colors_vario, 3);

    if (i < NUMSNAILCOLORS / 2 ||
        !settings_map.SnailScaling)
      iwidth = minwidth;
    else
      iwidth = max(minwidth, (i - NUMSNAILCOLORS / 2) *
                             Layout::Scale(16) / NUMSNAILCOLORS);

    hpSnail[i].set(minwidth, color);
    hpSnailVario[i].set(iwidth, color);
  }
}

void
Graphics::InitLandableIcons(const WaypointRendererSettings &settings)
{
  if (settings.landable_style == wpLandableWinPilot) {
    AirportReachableIcon.load_big(IDB_REACHABLE, IDB_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_MARGINAL, IDB_MARGINAL_HD);
    AirportUnreachableIcon.load_big(IDB_LANDABLE, IDB_LANDABLE_HD);
    FieldReachableIcon.load_big(IDB_REACHABLE, IDB_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_MARGINAL, IDB_MARGINAL_HD);
    FieldUnreachableIcon.load_big(IDB_LANDABLE, IDB_LANDABLE_HD);
  } else if (settings.landable_style == wpLandableAltA) {
    AirportReachableIcon.load_big(IDB_AIRPORT_REACHABLE,
                                  IDB_AIRPORT_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_AIRPORT_MARGINAL,
                                 IDB_AIRPORT_MARGINAL_HD);
    AirportUnreachableIcon.load_big(IDB_AIRPORT_UNREACHABLE,
                                    IDB_AIRPORT_UNREACHABLE_HD);
    FieldReachableIcon.load_big(IDB_OUTFIELD_REACHABLE,
                                IDB_OUTFIELD_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_OUTFIELD_MARGINAL,
                               IDB_OUTFIELD_MARGINAL_HD);
    FieldUnreachableIcon.load_big(IDB_OUTFIELD_UNREACHABLE,
                                  IDB_OUTFIELD_UNREACHABLE_HD);
  } else if (settings.landable_style == wpLandableAltB) {
    AirportReachableIcon.load_big(IDB_AIRPORT_REACHABLE,
                                  IDB_AIRPORT_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_AIRPORT_MARGINAL2,
                                 IDB_AIRPORT_MARGINAL2_HD);
    AirportUnreachableIcon.load_big(IDB_AIRPORT_UNREACHABLE2,
                                    IDB_AIRPORT_UNREACHABLE2_HD);
    FieldReachableIcon.load_big(IDB_OUTFIELD_REACHABLE,
                                IDB_OUTFIELD_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_OUTFIELD_MARGINAL2,
                               IDB_OUTFIELD_MARGINAL2_HD);
    FieldUnreachableIcon.load_big(IDB_OUTFIELD_UNREACHABLE2,
                                  IDB_OUTFIELD_UNREACHABLE2_HD);
  }
}

void
Graphics::Deinitialise()
{
  DeinitialiseUnitSymbols();

  hTerrainWarning.reset();
  hGPSStatus1.reset();
  hGPSStatus2.reset();
  hLogger.reset();
  hLoggerOff.reset();

  hCruise.reset();
  hClimb.reset();
  hFinalGlide.reset();
  hAbort.reset();

#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBrush.reset();
  hAboveTerrainBitmap.reset();
#endif

  hbWind.reset();

  hBmpMapScaleLeft.reset();
  hBmpMapScaleRight.reset();

  hBmpTabTask.reset();
  hBmpTabWrench.reset();
  hBmpTabSettings.reset();
  hBmpTabCalculator.reset();

  hBmpTabFlight.reset();
  hBmpTabSystem.reset();
  hBmpTabRules.reset();
  hBmpTabTimes.reset();

  hBmpThermalSource.reset();

  hBmpTrafficSafe.reset();
  hBmpTrafficWarning.reset();
  hBmpTrafficAlarm.reset();

  hbCompass.reset();

  hbFinalGlideBelow.reset();
  hbFinalGlideBelowLandable.reset();
  hbFinalGlideAbove.reset();

  hpWind.reset();

  hpCompass.reset();

  hpFinalGlideBelow.reset();
  hpFinalGlideBelowLandable.reset();
  hpFinalGlideAbove.reset();

  hpMapScale.reset();
  hpTerrainLine.reset();
  hpTerrainLineThick.reset();

  TracePen.reset();
  ContestPen[0].reset();
  ContestPen[1].reset();
  ContestPen[2].reset();

  SmallIcon.reset();
  TurnPointIcon.reset();
  TaskTurnPointIcon.reset();
  MountainTopIcon.reset();
  BridgeIcon.reset();
  TunnelIcon.reset();
  TowerIcon.reset();
  PowerPlantIcon.reset();

  hpAircraft.reset();
  hpAircraftSimple1.reset();
  hpAircraftSimple2.reset();
  hpCanopy.reset();
  hbCanopy.reset();

  hbGreen.reset();
  hbWhite.reset();
  hbMagenta.reset();
  hbOrange.reset();
  hbRed.reset();
  hbLightGray.reset();
  hbNotReachableTerrain.reset();

  hbGround.reset();

  hpTrackBearingLine.reset();

  for (unsigned i = 0; i < NUMSNAILCOLORS; i++) {
    hpSnail[i].reset();
    hpSnailVario[i].reset();
  }

  AirportReachableIcon.reset();
  AirportUnreachableIcon.reset();
  AirportMarginalIcon.reset();
  FieldMarginalIcon.reset();
  FieldReachableIcon.reset();
  FieldUnreachableIcon.reset();
}

static void
DrawMirroredPolygon(const RasterPoint *src, RasterPoint *dst, unsigned points,
                    Canvas &canvas, const Angle angle,
                    const RasterPoint pos)
{
  std::copy(src, src + points, dst);
  for (unsigned i = 0; i < points; ++i) {
    dst[2 * points - i - 1].x = -dst[i].x;
    dst[2 * points - i - 1].y = dst[i].y;
  }
  PolygonRotateShift(dst, 2 * points, pos.x, pos.y, angle, false);
  canvas.polygon(dst, 2 * points);
}


static void
DrawDetailedAircraft(Canvas &canvas, const SETTINGS_MAP &settings_map,
                     const Angle angle,
                     const RasterPoint aircraft_pos)
{
  {
    static const RasterPoint Aircraft[] = {
      {0, -10},
      {-2, -7},
      {-2, -2},
      {-16, -2},
      {-32, -1},
      {-32, 2},
      {-1, 3},
      {-1, 15},
      {-3, 15},
      {-5, 17},
      {-5, 18},
      {0, 18},
    };
    const unsigned AIRCRAFT_POINTS = sizeof(Aircraft) / sizeof(Aircraft[0]);
    RasterPoint buffer[2 * AIRCRAFT_POINTS];

    if (settings_map.terrain.enable) {
      canvas.white_brush();
      canvas.select(Graphics::hpAircraft);
    } else {
      canvas.black_brush();
      canvas.white_pen();
    }

    DrawMirroredPolygon(Aircraft, buffer, AIRCRAFT_POINTS,
                        canvas, angle, aircraft_pos);
  }

  {
    static const RasterPoint Canopy[] = {
      {0, -7},
      {-1, -7},
      {-1, -2},
      {0, -1},
    };
    const unsigned CANOPY_POINTS = sizeof(Canopy) / sizeof(Canopy[0]);
    RasterPoint buffer[2 * CANOPY_POINTS];

    canvas.select(Graphics::hpCanopy);
    canvas.select(Graphics::hbCanopy);
    DrawMirroredPolygon(Canopy, buffer, CANOPY_POINTS,
                        canvas, angle, aircraft_pos);
  }
}


static void
DrawSimpleAircraft(Canvas &canvas, const Angle angle,
                   const RasterPoint aircraft_pos, bool large)
{
  static const RasterPoint AircraftLarge[] = {
    {1, -7},
    {1, -1},
    {17, -1},
    {17, 1},
    {1, 1},
    {1, 10},
    {5, 10},
    {5, 12},
    {-5, 12},
    {-5, 10},
    {-1, 10},
    {-1, 1},
    {-17, 1},
    {-17, -1},
    {-1, -1},
    {-1, -7},
  };

  static const RasterPoint AircraftSmall[] = {
    {1, -5},
    {1, 0},
    {14, 0},
    {14, 1},
    {1, 1},
    {1, 8},
    {4, 8},
    {4, 9},
    {-3, 9},
    {-3, 8},
    {0, 8},
    {0, 1},
    {-13, 1},
    {-13, 0},
    {0, 0},
    {0, -5},
   };

  const unsigned AIRCRAFT_POINTS_LARGE =
                            sizeof(AircraftLarge) / sizeof(AircraftLarge[0]);
  const unsigned AIRCRAFT_POINTS_SMALL =
                            sizeof(AircraftSmall) / sizeof(AircraftSmall[0]);

  const RasterPoint *Aircraft = large ? AircraftLarge : AircraftSmall;
  const unsigned AircraftPoints = large ?
                                  AIRCRAFT_POINTS_LARGE : AIRCRAFT_POINTS_SMALL;

  RasterPoint aircraft[std::max(AIRCRAFT_POINTS_LARGE, AIRCRAFT_POINTS_SMALL)];
  std::copy(Aircraft, Aircraft + AircraftPoints, aircraft);
  PolygonRotateShift(aircraft, AircraftPoints,
                     aircraft_pos.x, aircraft_pos.y, angle, true);
  canvas.select(Graphics::hpAircraftSimple2);
  canvas.polygon(aircraft, AircraftPoints);
  canvas.black_brush();
  canvas.select(Graphics::hpAircraftSimple1);
  canvas.polygon(aircraft, AircraftPoints);
}


void
Graphics::DrawAircraft(Canvas &canvas, const SETTINGS_MAP &settings_map,
                       const Angle angle,
                       const RasterPoint aircraft_pos)
{
  switch (Appearance.AircraftSymbol) {
    case acDetailed:
      DrawDetailedAircraft(canvas, settings_map, angle, aircraft_pos);
      break;
    case acSimpleLarge:
      DrawSimpleAircraft(canvas, angle, aircraft_pos, true);
      break;
    case acSimple:
    default:
      DrawSimpleAircraft(canvas, angle, aircraft_pos, false);
      break;
  }
}
