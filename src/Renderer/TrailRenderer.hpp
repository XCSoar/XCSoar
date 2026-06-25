// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/AllocatedArray.hxx"
#include "Computer/TraceComputer.hpp"
#include "Engine/Trace/Point.hpp"
#include "Engine/Trace/Vector.hpp"
#include "ui/dim/Point.hpp"
#include "time/Stamp.hpp"

#include <vector>

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
  struct TrailPointData {
    PixelPoint point;
    double value{};
    TracePoint::Time time{};
  };

  const TrailLook &look;

  TracePointVector trace;
  std::vector<TrailVarioSample> merge_vario_samples;
  AllocatedArray<BulkPixelPoint> points;

  /** Reused each Draw() to avoid per-frame heap allocations. */
  std::vector<TrailPointData> valid_points;
  std::vector<PixelPoint> interpolated;
  std::vector<std::pair<TracePoint::Time, double>> vario_knots;

public:
  TrailRenderer(const TrailLook &_look) noexcept:look(_look) {}

  /**
   * Load the full trace into this object.
   */
  bool LoadTrace(const TraceComputer &trace_computer) noexcept;

  /**
   * Load a filtered trace into this object.
   */
  bool LoadTrace(const TraceComputer &trace_computer,
                 TimeStamp min_time,
                 const WindowProjection &projection,
                 double map_scale) noexcept;

  void ScanBounds(GeoBounds &bounds) const noexcept {
    trace.ScanBounds(bounds);
  }

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection,
            TimeStamp min_time,
            bool enable_traildrift, PixelPoint pos, const NMEAInfo &basic,
            const DerivedInfo &calculated,
            const TrailSettings &settings) noexcept;

  /**
   * Draw the trace that was obtained by LoadTrace() with the trace pen.
   */
  void Draw(Canvas &canvas, const WindowProjection &projection) noexcept;

  void Draw(Canvas &canvas, const TraceComputer &trace_computer,
            const WindowProjection &projection,
            TimeStamp min_time={}) noexcept;

  [[gnu::malloc]]
  BulkPixelPoint *Prepare(unsigned n) noexcept;

  void DrawPreparedPolyline(Canvas &canvas, unsigned n) noexcept;
  void DrawPreparedPolygon(Canvas &canvas, unsigned n) noexcept;

  /**
   * Draw a ContestTraceVector.  The caller must select a Pen.
   */
  void DrawTraceVector(Canvas &canvas, const Projection &projection,
                       const ContestTraceVector &trace) noexcept;

  /**
   * Draw a triangle described by trace index 1..3; expected trace
   * size is 5 (0=start, 4=finish).
   */
  void DrawTriangle(Canvas &canvas, const Projection &projection,
                    const ContestTraceVector &trace) noexcept;

private:
  void SelectTrailPen(Canvas &canvas, unsigned color_index,
                      bool scaled_trail) const noexcept;

  void PrepareCopy(const PixelPoint *src, unsigned n) noexcept;

  void DrawColourPolyline(Canvas &canvas, unsigned color_index,
                          bool scaled_trail,
                          const std::vector<PixelPoint> &pts,
                          size_t first, size_t last) noexcept;

  void DrawTraceVector(Canvas &canvas, const Projection &projection,
                       const TracePointVector &trace) noexcept;
};
