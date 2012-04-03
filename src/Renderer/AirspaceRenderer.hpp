/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_AIRSPACE_RENDERER_HPP
#define XCSOAR_AIRSPACE_RENDERER_HPP

#include "Util/StaticArray.hpp"
#include "Engine/Navigation/GeoPoint.hpp"

struct AirspaceLook;
struct MoreData;
struct DerivedInfo;
struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
class Airspaces;
class AirspacePredicate;
class ProtectedAirspaceWarningManager;
class AirspaceWarningCopy;
class Canvas;
class WindowProjection;

class AirspaceRenderer
{
  const AirspaceLook &airspace_look;

  const Airspaces *airspace_database;
  const ProtectedAirspaceWarningManager *airspace_warnings;

  StaticArray<GeoPoint,32> m_airspace_intersections;

public:
  AirspaceRenderer(const AirspaceLook &_airspace_look)
    :airspace_look(_airspace_look),
     airspace_database(NULL), airspace_warnings(NULL)
  {}

  const AirspaceLook &GetLook() const {
    return airspace_look;
  }

  const Airspaces *GetAirspaces() const {
    return airspace_database;
  }

  const ProtectedAirspaceWarningManager *GetAirspaceWarnings() const {
    return airspace_warnings;
  }

  void SetAirspaces(const Airspaces *_airspace_database) {
    airspace_database = _airspace_database;
  }

  void SetAirspaceWarnings(const ProtectedAirspaceWarningManager *_airspace_warnings) {
    airspace_warnings = _airspace_warnings;
  }

  void Clear() {
    airspace_database = NULL;
    airspace_warnings = NULL;
  }

  /**
   * Draw airspaces selected by the given #AirspacePredicate.
   */
  void Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
            Canvas &buffer_canvas, Canvas &stencil_canvas,
#endif
            const WindowProjection &projection,
            const AirspaceRendererSettings &settings,
            const AirspaceWarningCopy &awc,
            const AirspacePredicate &visible);

  /**
   * Draw all airspaces.
   */
  void Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
            Canvas &buffer_canvas, Canvas &stencil_canvas,
#endif
            const WindowProjection &projection,
            const AirspaceRendererSettings &settings);

  /**
   * Draw airspaces that are visible according to standard rules.
   */
  void Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
            Canvas &buffer_canvas, Canvas &stencil_canvas,
#endif
            const WindowProjection &projection,
            const MoreData &basic, const DerivedInfo &calculated,
            const AirspaceComputerSettings &computer_settings,
            const AirspaceRendererSettings &settings);

  void DrawIntersections(Canvas &canvas,
                         const WindowProjection &projection) const;
};

#endif
