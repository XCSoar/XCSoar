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

#include "MapLook.hpp"
#include "Screen/Layout.hpp"
#include "resource.h"

void
MapLook::Initialise()
{
#ifdef HAVE_HATCHED_BRUSH
  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);
  hAboveTerrainBrush.Set(hAboveTerrainBitmap);
#endif

  hTerrainWarning.Load(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);

  hpWind.Set(Layout::Scale(1), DarkColor(COLOR_GRAY));
  hpWindTail.Set(Pen::DASH, 1, COLOR_BLACK);
  hbWind.Set(COLOR_GRAY);

  hbCompass.Set(Color(207, 207, 207));
  hpCompass.Set(Layout::Scale(1), COLOR_GRAY);

  hBmpTrafficSafe.Load(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  hBmpTrafficWarning.Load(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  hBmpTrafficAlarm.Load(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  static gcc_constexpr_data Color clrSepia(0x78,0x31,0x18);
  hpTerrainLine.Set(Pen::DASH, Layout::Scale(1), clrSepia);
  hpTerrainLineThick.Set(Pen::DASH, Layout::Scale(2), clrSepia);

  hpTrackBearingLine.Set(3, COLOR_GRAY);

  ContestPen[0].Set(Layout::Scale(1)+2, COLOR_RED);
  ContestPen[1].Set(Layout::Scale(1)+1, COLOR_ORANGE);
  ContestPen[2].Set(Layout::Scale(1), COLOR_BLUE);

  hBmpThermalSource.Load(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);

  hBmpTrafficSafe.Load(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  hBmpTrafficWarning.Load(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  hBmpTrafficAlarm.Load(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  hBmpMapScaleLeft.Load(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  hBmpMapScaleRight.Load(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);
}
