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
Pen Graphics::hpTerrainLine;
Pen Graphics::hpTerrainLineThick;
Pen Graphics::hpTrackBearingLine;
Pen Graphics::TracePen;
Pen Graphics::ContestPen[3];

Brush Graphics::hbCompass;
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

  hTerrainWarning.Load(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);
  hGPSStatus1.Load(IDB_GPSSTATUS1, IDB_GPSSTATUS1_HD, false);
  hGPSStatus2.Load(IDB_GPSSTATUS2, IDB_GPSSTATUS2_HD, false);
  hLogger.Load(IDB_LOGGER, IDB_LOGGER_HD);
  hLoggerOff.Load(IDB_LOGGEROFF, IDB_LOGGEROFF_HD);

  hCruise.Load(IDB_CRUISE, IDB_CRUISE_HD, false);
  hClimb.Load(IDB_CLIMB, IDB_CLIMB_HD, false);
  hFinalGlide.Load(IDB_FINALGLIDE, IDB_FINALGLIDE_HD, false);
  hAbort.Load(IDB_ABORT, IDB_ABORT_HD, false);

#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);

  hAboveTerrainBrush.Set(hAboveTerrainBitmap);
#endif

  hpWind.Set(Layout::Scale(1), DarkColor(COLOR_GRAY));
  hpWindTail.Set(Pen::DASH, 1, COLOR_BLACK);
  hbWind.Set(COLOR_GRAY);

  hBmpMapScaleLeft.Load(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  hBmpMapScaleRight.Load(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);

  hBmpTabTask.load((Layout::scale > 1) ? IDB_TASK_HD : IDB_TASK);
  hBmpTabWrench.load((Layout::scale > 1) ? IDB_WRENCH_HD : IDB_WRENCH);
  hBmpTabSettings.load((Layout::scale > 1) ? IDB_SETTINGS_HD : IDB_SETTINGS);
  hBmpTabCalculator.load((Layout::scale > 1) ? IDB_CALCULATOR_HD : IDB_CALCULATOR);

  hBmpTabFlight.load((Layout::scale > 1) ? IDB_GLOBE_HD : IDB_GLOBE);
  hBmpTabSystem.load((Layout::scale > 1) ? IDB_DEVICE_HD : IDB_DEVICE);
  hBmpTabRules.load((Layout::scale > 1) ? IDB_RULES_HD : IDB_RULES);
  hBmpTabTimes.load((Layout::scale > 1) ? IDB_CLOCK_HD : IDB_CLOCK);

  hBmpThermalSource.Load(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);

  hBmpTrafficSafe.Load(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  hBmpTrafficWarning.Load(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  hBmpTrafficAlarm.Load(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  hbCompass.Set(Color(207, 207, 207));

  hpCompass.Set(Layout::Scale(1), COLOR_GRAY);

  hpTerrainLine.Set(Pen::DASH, Layout::Scale(1), clrSepia);
  hpTerrainLineThick.Set(Pen::DASH, Layout::Scale(2), clrSepia);

  TracePen.Set(2, Color(50, 243, 45));
  ContestPen[0].Set(Layout::Scale(1)+2, COLOR_RED);
  ContestPen[1].Set(Layout::Scale(1)+1, COLOR_ORANGE);
  ContestPen[2].Set(Layout::Scale(1), COLOR_BLUE);

  hbGround.Set(GroundColor);

  hpTrackBearingLine.Set(3, COLOR_GRAY);
}

void
Graphics::Deinitialise()
{
  DeinitialiseUnitSymbols();

  hTerrainWarning.Reset();
  hGPSStatus1.Reset();
  hGPSStatus2.Reset();
  hLogger.Reset();
  hLoggerOff.Reset();

  hCruise.Reset();
  hClimb.Reset();
  hFinalGlide.Reset();
  hAbort.Reset();

#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBrush.Reset();
  hAboveTerrainBitmap.reset();
#endif

  hbWind.Reset();

  hBmpMapScaleLeft.Reset();
  hBmpMapScaleRight.Reset();

  hBmpTabTask.reset();
  hBmpTabWrench.reset();
  hBmpTabSettings.reset();
  hBmpTabCalculator.reset();

  hBmpTabFlight.reset();
  hBmpTabSystem.reset();
  hBmpTabRules.reset();
  hBmpTabTimes.reset();

  hBmpThermalSource.Reset();

  hBmpTrafficSafe.Reset();
  hBmpTrafficWarning.Reset();
  hBmpTrafficAlarm.Reset();

  hbCompass.Reset();

  hpWind.Reset();
  hpWindTail.Reset();

  hpCompass.Reset();

  hpTerrainLine.Reset();
  hpTerrainLineThick.Reset();

  TracePen.Reset();
  ContestPen[0].Reset();
  ContestPen[1].Reset();
  ContestPen[2].Reset();

  hbGround.Reset();

  hpTrackBearingLine.Reset();
}
