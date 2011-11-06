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

#ifndef XCSOAR_TRAIL_RENDERER_HPP
#define XCSOAR_TRAIL_RENDERER_HPP

#include "Util/AllocatedArray.hpp"
#include "Screen/Point.hpp"
#include "Engine/Navigation/TracePoint.hpp"

class Canvas;
class TraceComputer;
class Projection;
class WindowProjection;
class ContestTraceVector;
struct TrailLook;
struct NMEAInfo;
struct DerivedInfo;
struct SETTINGS_MAP;

class TrailRenderer {
  const TrailLook &look;

  TracePointVector trace;
  AllocatedArray<RasterPoint> points;

public:
  TrailRenderer(const TrailLook &_look):look(_look) {}

  /**
   * Load the full trace into this object.
   */
  bool LoadTrace(const TraceComputer &trace_computer);

  /**
   * Load a filtered trace into this object.
   */
  bool LoadTrace(const TraceComputer &trace_computer, unsigned min_time,
                 const WindowProjection &projection);

  gcc_pure
  TaskProjection GetBounds(const GeoPoint fallback_location) const;

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection, unsigned min_time,
            bool enable_traildrift, const RasterPoint pos, const NMEAInfo &basic,
            const DerivedInfo &calculated, const SETTINGS_MAP &settings);

  /**
   * Draw the trace that was obtained by LoadTrace() with the trace pen.
   */
  void Draw(Canvas &canvas, const WindowProjection &projection);

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection, unsigned min_time);

  /**
   * Draw a ContestTraceVector.  The caller must select a Pen.
   */
  void Draw(Canvas &canvas, const WindowProjection &projection,
            const ContestTraceVector &trace);

private:
  void DrawTraceVector(Canvas &canvas, const Projection &projection,
                       const TracePointVector &trace);
};

#endif
