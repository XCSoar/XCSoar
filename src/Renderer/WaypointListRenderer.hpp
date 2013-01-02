/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_WAYPOINT_LIST_RENDERER_HPP
#define XCSOAR_WAYPOINT_LIST_RENDERER_HPP

#include "Screen/Point.hpp"
#include "Math/fixed.hpp"

class Canvas;
struct Waypoint;
struct GeoVector;
struct DialogLook;
struct WaypointLook;
struct WaypointRendererSettings;

namespace WaypointListRenderer
{
  gcc_pure
  UPixelScalar GetHeight(const DialogLook &dialog_look);

  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            const DialogLook &dialog_look, const WaypointLook &look,
            const WaypointRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            const GeoVector &vector,
            const DialogLook &dialog_look, const WaypointLook &look,
            const WaypointRendererSettings &settings);

  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            fixed distance, fixed arrival_altitude,
            const DialogLook &dialog_look, const WaypointLook &look,
            const WaypointRendererSettings &settings);
}

#endif
