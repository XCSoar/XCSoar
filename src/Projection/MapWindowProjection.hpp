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

#ifndef XCSOAR_MAPWINDOW_PROJECTION_HPP
#define XCSOAR_MAPWINDOW_PROJECTION_HPP

#include "WindowProjection.hpp"
#include "Compiler.h"

struct Waypoint;

class MapWindowProjection:
  public WindowProjection
{
public:
  /**
   * Sets a map scale which is not affected by the hard-coded scale
   * list.
   */
  void SetFreeMapScale(double x);

  void SetMapScale(double x);

public:
  bool HaveScaleList() const {
    return true;
  }

  /**
   * Calculates a scale index.
   */
  gcc_pure
  double CalculateMapScale(unsigned scale) const;

  gcc_pure
  double StepMapScale(double scale, int Step) const;

  gcc_pure
  bool WaypointInScaleFilter(const Waypoint &way_point) const;

private:
  double LimitMapScale(double value) const;

  gcc_pure
  unsigned FindMapScale(double Value) const;
};

#endif
