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

#include "MapWindow.hpp"
#include "Math/Earth.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/ProtectedTaskManager.hpp"

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  const MapWindowProjection &projection = render_projection;

  if (GetDisplayMode() == dmCircling) {
    if (Calculated().ThermalEstimate_Valid) {
      RasterPoint sc;
      if (projection.GeoToScreenIfVisible(Calculated().ThermalEstimate_Location, sc)) {
        Graphics::hBmpThermalSource.draw(canvas, sc);
      }
    }
  } else if (projection.GetMapScale() <= fixed(4000)) {
    for (int i = 0; i < MAX_THERMAL_SOURCES; i++) {
      if (!positive(Calculated().ThermalSources[i].LiftRate))
        continue;

      fixed dh =
          Basic().NavAltitude - Calculated().ThermalSources[i].GroundHeight;
      if (negative(dh))
        continue;

      fixed t = -dh / Calculated().ThermalSources[i].LiftRate;
      GeoPoint loc =
          FindLatitudeLongitude(Calculated().ThermalSources[i].Location,
                                Basic().wind.bearing, Basic().wind.norm * t);

      RasterPoint pt;
      if (render_projection.GeoToScreenIfVisible(loc, pt))
        Graphics::hBmpThermalSource.draw(canvas, pt);
    }
  }
}
