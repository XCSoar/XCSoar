/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Waypoint/Waypoint.hpp"
#include "Util/Macros.hpp"
#include "Util/Clamp.hpp"

#include <assert.h>

static constexpr unsigned ScaleList[] = {
  100,
  200,
  300,
  500,
  1000,
  2000,
  3000,
  5000,
  10000,
  20000,
  30000,
  50000,
  75000,
  100000,
  150000,
  200000,
  300000,
  500000,
  1000000,
};

static constexpr unsigned ScaleListCount = ARRAY_SIZE(ScaleList);

bool
MapWindowProjection::WaypointInScaleFilter(const Waypoint &way_point) const
{
  return (GetMapScale() <= (way_point.IsLandable() ? 20000 : 10000));
}

double
MapWindowProjection::CalculateMapScale(unsigned scale) const
{
  assert(scale < ScaleListCount);
  return double(ScaleList[scale]) *
    GetMapResolutionFactor() / Layout::Scale(GetScreenWidth());
}

double
MapWindowProjection::LimitMapScale(const double value) const
{
  return HaveScaleList() ? CalculateMapScale(FindMapScale(value)) : value;
}

double
MapWindowProjection::StepMapScale(const double scale, int Step) const
{
  int i = FindMapScale(scale) + Step;
  i = Clamp(i, 0, (int)ScaleListCount - 1);
  return CalculateMapScale(i);
}

unsigned
MapWindowProjection::FindMapScale(const double Value) const
{
  unsigned DesiredScale(Value * Layout::Scale(GetScreenWidth())
                        / GetMapResolutionFactor());

  unsigned i;
  for (i = 0; i < ScaleListCount; i++) {
    if (DesiredScale < ScaleList[i]) {
      if (i == 0)
        return 0;

      return i - (DesiredScale < (ScaleList[i] + ScaleList[i - 1]) / 2);
    }
  }

  return ScaleListCount - 1;
}

void
MapWindowProjection::SetFreeMapScale(const double x)
{
  SetScale(double(GetMapResolutionFactor()) / x);
}

void
MapWindowProjection::SetMapScale(const double x)
{
  SetScale(double(GetMapResolutionFactor()) / LimitMapScale(x));
}
