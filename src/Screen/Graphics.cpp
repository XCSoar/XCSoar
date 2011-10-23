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

Pen Graphics::hpWind;
Pen Graphics::hpWindTail;
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

Brush Graphics::hbCompass;
Brush Graphics::hbFinalGlideBelow;
Brush Graphics::hbFinalGlideBelowLandable;
Brush Graphics::hbFinalGlideAbove;
Brush Graphics::hbWind;

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
Brush Graphics::hbOrange;
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

  hAboveTerrainBrush.Set(hAboveTerrainBitmap);
#endif

  hpWind.set(Layout::Scale(1), DarkColor(COLOR_GRAY));
  hpWindTail.set(Pen::DASH, 1, COLOR_BLACK);
  hbWind.Set(COLOR_GRAY);

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

  hbCompass.Set(Color(207, 207, 207));

  hbFinalGlideBelow.Set(COLOR_RED);
  hpFinalGlideBelow.set(Layout::Scale(1), DarkColor(COLOR_RED));

  hbFinalGlideBelowLandable.Set(COLOR_ORANGE);
  hpFinalGlideBelowLandable.set(Layout::Scale(1), DarkColor(COLOR_ORANGE));

  hbFinalGlideAbove.Set(COLOR_GREEN);
  hpFinalGlideAbove.set(Layout::Scale(1), DarkColor(COLOR_GREEN));

  hpCompass.set(Layout::Scale(1), COLOR_GRAY);

  hpMapScale.set(Layout::Scale(1), COLOR_BLACK);

  hpTerrainLine.set(Pen::DASH, Layout::Scale(1), clrSepia);
  hpTerrainLineThick.set(Pen::DASH, Layout::Scale(2), clrSepia);

  TracePen.set(2, Color(50, 243, 45));
  ContestPen[0].set(Layout::Scale(1)+2, COLOR_RED);
  ContestPen[1].set(Layout::Scale(1)+1, COLOR_ORANGE);
  ContestPen[2].set(Layout::Scale(1), COLOR_BLUE);

    // used for landable rendering
  hbGreen.Set(COLOR_GREEN);
  hbWhite.Set(COLOR_WHITE);
  hbOrange.Set(COLOR_ORANGE);
  hbLightGray.Set(COLOR_LIGHT_GRAY);
  hbNotReachableTerrain.Set(LightColor(COLOR_RED));

  hbGround.Set(GroundColor);

  hpTrackBearingLine.set(3, COLOR_GRAY);
}

void
Graphics::InitialiseConfigured(const SETTINGS_MAP &settings_map)
{
  InitSnailTrail(settings_map);
}

void
Graphics::InitSnailTrail(const SETTINGS_MAP &settings_map)
{
  static gcc_constexpr_data ColorRamp snail_colors_vario[] = {
    {0,   0xc4, 0x80, 0x1e}, // sinkColor
    {100, 0xa0, 0xa0, 0xa0},
    {200, 0x1e, 0xf1, 0x73} // liftColor
  };

  static gcc_constexpr_data ColorRamp snail_colors_vario2[] = {
    {0,   0x00, 0x00, 0xff},
    {99,  0x00, 0xff, 0xff},
    {100, 0xff, 0xff, 0x00},
    {200, 0xff, 0x00, 0x00}
  };

  static gcc_constexpr_data ColorRamp snail_colors_alt[] = {
    {0,   0xff, 0x00, 0x00},
    {50,  0xff, 0xff, 0x00},
    {100, 0x00, 0xff, 0x00},
    {150, 0x00, 0xff, 0xff},
    {200, 0x00, 0x00, 0xff},
  };

  PixelScalar iwidth;
  PixelScalar minwidth = Layout::Scale(2);

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
      iwidth = max(minwidth,
                   PixelScalar((i - NUMSNAILCOLORS / 2) *
                               Layout::Scale(16) / NUMSNAILCOLORS));

    hpSnail[i].set(minwidth, color);
    hpSnailVario[i].set(iwidth, color);
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
  hAboveTerrainBrush.Reset();
  hAboveTerrainBitmap.reset();
#endif

  hbWind.Reset();

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

  hbCompass.Reset();

  hbFinalGlideBelow.Reset();
  hbFinalGlideBelowLandable.Reset();
  hbFinalGlideAbove.Reset();

  hpWind.reset();
  hpWindTail.reset();

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

  hbGreen.Reset();
  hbWhite.Reset();
  hbOrange.Reset();
  hbLightGray.Reset();
  hbNotReachableTerrain.Reset();

  hbGround.Reset();

  hpTrackBearingLine.reset();

  for (unsigned i = 0; i < NUMSNAILCOLORS; i++) {
    hpSnail[i].reset();
    hpSnailVario[i].reset();
  }
}
