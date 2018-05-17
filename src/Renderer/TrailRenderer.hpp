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

#ifndef XCSOAR_TRAIL_RENDERER_HPP
#define XCSOAR_TRAIL_RENDERER_HPP

#include "Util/AllocatedArray.hxx"
#include "Engine/Trace/Point.hpp"
#include "Engine/Trace/Vector.hpp"

struct PixelPoint;
struct BulkPixelPoint;
class Canvas;
class TraceComputer;
class Projection;
class WindowProjection;
class ContestTraceVector;
struct ContestTracePoint;
struct TrailLook;
struct NMEAInfo;
struct DerivedInfo;
struct TrailSettings;

/**
 * Trail renderer
 * renders the trail of past position fixes on the map
 * includes filter for coarse-graining trail in LoadTrace
 */
class TrailRenderer {
  const TrailLook &look;

  TracePointVector trace;
  AllocatedArray<BulkPixelPoint> points;

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

  void ScanBounds(GeoBounds &bounds) const {
    trace.ScanBounds(bounds);
  }

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection, unsigned min_time,
            bool enable_traildrift, PixelPoint pos, const NMEAInfo &basic,
            const DerivedInfo &calculated, const TrailSettings &settings);

  /**
   * Draw the trace that was obtained by LoadTrace() with the trace pen.
   */
  void Draw(Canvas &canvas, const WindowProjection &projection);

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection, unsigned min_time);

  gcc_malloc
  BulkPixelPoint *Prepare(unsigned n);

  void DrawPreparedPolyline(Canvas &canvas, unsigned n);
  void DrawPreparedPolygon(Canvas &canvas, unsigned n);

  /**
   * Draw a ContestTraceVector.  The caller must select a Pen.
   */
  void DrawTraceVector(Canvas &canvas, const Projection &projection,
                       const ContestTraceVector &trace);

  /**
   * Draw a triangle described by trace index 1..3; expected trace
   * size is 5 (0=start, 4=finish).
   */
  void DrawTriangle(Canvas &canvas, const Projection &projection,
                    const ContestTraceVector &trace);

private:
  void DrawTraceVector(Canvas &canvas, const Projection &projection,
                       const TracePointVector &trace);
};

#endif
