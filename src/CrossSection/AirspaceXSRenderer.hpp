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

#ifndef AIRSPACE_CROSS_SECTION_RENDERER_HPP
#define AIRSPACE_CROSS_SECTION_RENDERER_HPP

#include "Renderer/AirspaceRendererSettings.hpp"

struct AirspaceLook;
class Canvas;
class ChartRenderer;
class Airspaces;
struct GeoPoint;
struct GeoVector;
struct AircraftState;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class AirspaceXSRenderer
{
  AirspaceRendererSettings settings;

  const AirspaceLook &look;

public:
  AirspaceXSRenderer(const AirspaceLook &_look): look(_look) {}

  void Draw(Canvas &canvas, const ChartRenderer &chart,
            const Airspaces &database,
            const GeoPoint &start, const GeoVector &vec,
            const AircraftState &state) const;

  void SetSettings(const AirspaceRendererSettings &_settings) {
    settings = _settings;
  }
};

#endif
