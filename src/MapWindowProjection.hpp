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

#ifndef XCSOAR_MAPWINDOW_PROJECTION_HPP
#define XCSOAR_MAPWINDOW_PROJECTION_HPP

#include "WindowProjection.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

class Waypoint;

class MapWindowProjection:
  public WindowProjection
{
public:
  MapWindowProjection();

public:
  void Initialize();

  void RequestMapScale(const fixed x);

public:
  bool HaveScaleList() const {
    return ScaleListCount > 0;
  }

  /**
   * Calculates a scale index.
   */
  gcc_pure
  fixed CalculateMapScale(const int scale) const;

  gcc_pure
  fixed StepMapScale(const fixed scale, int Step) const;

  gcc_pure
  bool WaypointInScaleFilter(const Waypoint &way_point) const;

private:
  fixed LimitMapScale(const fixed value) const;

  gcc_pure
  int FindMapScale(const fixed Value) const;

  fixed ScaleList[11];
  int ScaleListCount;
};

#endif
