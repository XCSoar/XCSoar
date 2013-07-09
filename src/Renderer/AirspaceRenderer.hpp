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

#ifndef XCSOAR_AIRSPACE_RENDERER_HPP
#define XCSOAR_AIRSPACE_RENDERER_HPP

#include "Util/StaticArray.hpp"
#include "Geo/GeoPoint.hpp"

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
  const AirspaceLook &look;

  const Airspaces *airspaces;
  const ProtectedAirspaceWarningManager *warning_manager;

  StaticArray<GeoPoint,32> intersections;

public:
  AirspaceRenderer(const AirspaceLook &_look)
    :look(_look), airspaces(NULL), warning_manager(NULL) {}

  const AirspaceLook &GetLook() const {
    return look;
  }

  const Airspaces *GetAirspaces() const {
    return airspaces;
  }

  const ProtectedAirspaceWarningManager *GetWarningManager() const {
    return warning_manager;
  }

  void SetAirspaces(const Airspaces *_airspaces) {
    airspaces = _airspaces;
  }

  void SetAirspaceWarnings(const ProtectedAirspaceWarningManager *_warning_manager) {
    warning_manager = _warning_manager;
  }

  void Clear() {
    airspaces = NULL;
    warning_manager = NULL;
  }

#ifndef ENABLE_OPENGL
private:
  void DrawFill(Canvas &canvas,
                Canvas &buffer_canvas, Canvas &stencil_canvas,
                const WindowProjection &projection,
                const AirspaceRendererSettings &settings,
                const AirspaceWarningCopy &awc,
                const AirspacePredicate &visible);

  void DrawOutline(Canvas &canvas,
                   const WindowProjection &projection,
                   const AirspaceRendererSettings &settings,
                   const AirspacePredicate &visible) const;

public:
#endif

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
