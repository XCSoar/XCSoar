// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/AllocatedArray.hxx"
#include "Engine/Trace/Point.hpp"
#include "Engine/Trace/Vector.hpp"
#include "time/Stamp.hpp"

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
  bool LoadTrace(const TraceComputer &trace_computer,
                 TimeStamp min_time,
                 const WindowProjection &projection);

  void ScanBounds(GeoBounds &bounds) const {
    trace.ScanBounds(bounds);
  }

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection,
            TimeStamp min_time,
            bool enable_traildrift, PixelPoint pos, const NMEAInfo &basic,
            const DerivedInfo &calculated, const TrailSettings &settings);

  /**
   * Draw the trace that was obtained by LoadTrace() with the trace pen.
   */
  void Draw(Canvas &canvas, const WindowProjection &projection);

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection,
            TimeStamp min_time={}) noexcept;

  [[gnu::malloc]]
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
