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

#include "MapWindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "NMEA/Info.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Units.hpp"

#include <stdlib.h>
#include <math.h>

MapWindowProjection::MapWindowProjection():
  WindowProjection(),
  DisplayMode(dmCruise),
  MapScale(5)
{
  ScaleList[0] = fixed_half;
  ScaleList[1] = fixed_one;
  ScaleList[2] = fixed_two;
  ScaleList[3] = fixed(5);
  ScaleList[4] = fixed_ten;
  ScaleList[5] = fixed(20);
  ScaleList[6] = fixed(50);
  ScaleList[7] = fixed(100);
  ScaleList[8] = fixed(200);
  ScaleList[9] = fixed(500);
  ScaleList[10] = fixed(1000);
  ScaleListCount = 11;
}

void
MapWindowProjection::Initialize(const SETTINGS_MAP &settings_map,
                                         const RECT &rc)
{
  MapRect = rc;

  RequestMapScaleUser(MapScale, settings_map);
}

bool
MapWindowProjection::WaypointInScaleFilter(const Waypoint &way_point) const
{
  return (MapScale <= (way_point.is_landable() ? fixed_int_constant(20) :
                                                 fixed_ten));
}

fixed
MapWindowProjection::CalculateMapScale(int scale) const
{
  return Units::ToSysDistance(CalculateMapScaleUser(scale));
}

fixed
MapWindowProjection::CalculateMapScaleUser(int scale) const
{
  assert(scale >= 0 && scale < ScaleListCount);

  return ScaleList[scale] * GetMapResolutionFactor() /
         Layout::Scale(GetScreenWidth());
}

fixed
MapWindowProjection::LimitMapScale(fixed value,
                                   const SETTINGS_MAP& settings_map)
{
  return Units::ToSysDistance(LimitMapScaleUser(Units::ToUserDistance(value),
                                                settings_map));
}

fixed
MapWindowProjection::LimitMapScaleUser(fixed value,
                                       const SETTINGS_MAP& settings_map)
{
  fixed minreasonable = fixed(0.05);

  if (settings_map.AutoZoom && DisplayMode != dmCircling)
    minreasonable = fixed(0.44);

  value = max(minreasonable, min(fixed_int_constant(160), value));
  if (HaveScaleList())
    value = CalculateMapScaleUser(FindMapScaleUser(value));

  return value;
}

fixed
MapWindowProjection::StepMapScale(fixed scale, int Step) const
{
  int i = FindMapScale(scale) + Step;
  i = max(0, min(ScaleListCount - 1, i));
  return CalculateMapScale(i);
}

int
MapWindowProjection::FindMapScale(const fixed Value) const
{
  return FindMapScaleUser(Units::ToUserDistance(Value));
}

int
MapWindowProjection::FindMapScaleUser(const fixed Value) const
{
  fixed BestFit;
  int BestFitIdx = 0;
  fixed DesiredScale = Value * Layout::Scale(GetScreenWidth()) /
                       GetMapResolutionFactor();

  for (int i = 0; i < ScaleListCount; i++) {
    fixed err = fabs(DesiredScale - ScaleList[i]) / DesiredScale;
    if (i == 0 || err < BestFit) {
      BestFit = err;
      BestFitIdx = i;
    }
  }

  return BestFitIdx;
}

void
MapWindowProjection::RequestMapScaleUser(fixed x,
                                         const SETTINGS_MAP &settings_map)
{
  MapScale = LimitMapScaleUser(x, settings_map);
  SetScale(fixed(GetMapResolutionFactor()) / Units::ToSysDistance(MapScale));
}

void
MapWindowProjection::RequestMapScale(fixed x, const SETTINGS_MAP &settings_map)
{
  RequestMapScaleUser(Units::ToUserDistance(x), settings_map);
}
