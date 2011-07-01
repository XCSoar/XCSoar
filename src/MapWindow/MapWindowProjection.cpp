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

#include "MapWindowProjection.hpp"
#include "Screen/Layout.hpp"
#include "Waypoint/Waypoint.hpp"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

static const unsigned ScaleList[] = {
  500,
  1000,
  2000,
  3000,
  5000,
  10000,
  20000,
  30000,
  50000,
  100000,
  200000,
  300000,
  500000,
  1000000,
};

static const unsigned ScaleListCount =
  sizeof(ScaleList) / sizeof(ScaleList[0]);

bool
MapWindowProjection::WaypointInScaleFilter(const Waypoint &way_point) const
{
  return (GetMapScale() <= (way_point.IsLandable() ? fixed_int_constant(20000) :
                                                      fixed_int_constant(10000)));
}

fixed
MapWindowProjection::CalculateMapScale(unsigned scale) const
{
  assert(scale < ScaleListCount);
  return fixed(ScaleList[scale]) *
    GetMapResolutionFactor() / Layout::Scale(GetScreenWidth());
}

fixed
MapWindowProjection::LimitMapScale(const fixed value) const
{
  return HaveScaleList() ? CalculateMapScale(FindMapScale(value)) : value;
}

fixed
MapWindowProjection::StepMapScale(const fixed scale, int Step) const
{
  int i = FindMapScale(scale) + Step;
  i = max(0, min((int)ScaleListCount - 1, i));
  return CalculateMapScale(i);
}

unsigned
MapWindowProjection::FindMapScale(const fixed Value) const
{
  unsigned DesiredScale = Value *
                       Layout::Scale(GetScreenWidth()) / GetMapResolutionFactor();

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
MapWindowProjection::SetFreeMapScale(const fixed x)
{
  SetScale(fixed(GetMapResolutionFactor()) / x);
}

void
MapWindowProjection::SetMapScale(const fixed x)
{
  SetScale(fixed(GetMapResolutionFactor()) / LimitMapScale(x));
}
