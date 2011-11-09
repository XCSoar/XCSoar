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

#include "MapWindow.hpp"
#include "Look/MapLook.hpp"
#include "Math/Earth.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/ProtectedTaskManager.hpp"

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  const MapWindowProjection &projection = render_projection;
  if (projection.GetMapScale() > fixed(4000))
    return;

  // draw only at close map scales in non-circling mode
  // draw thermal at location it would be at the glider's height

  const ThermalLocatorInfo &thermal_locator = Calculated().thermal_locator;

  for (auto it = thermal_locator.sources.begin(),
       end = thermal_locator.sources.end(); it != end; ++it) {
    // find height difference
    if (Basic().nav_altitude < it->ground_height)
      continue;

    GeoPoint loc =
        Calculated().wind_available ?
        it->CalculateAdjustedLocation(Basic().nav_altitude, Calculated().wind) :
        it->location;

    // draw if it is in the field of view
    RasterPoint pt;
    if (render_projection.GeoToScreenIfVisible(loc, pt))
      look.hBmpThermalSource.Draw(canvas, pt);
  }
}
