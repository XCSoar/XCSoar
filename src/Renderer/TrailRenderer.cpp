// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrailRenderer.hpp"
#include "Look/TrailLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "MapSettings.hpp"
#include "Computer/TraceComputer.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/Math.hpp"
#include "Engine/Contest/ContestTrace.hpp"
#include "Math/Constants.hpp"

#include <algorithm>
#include <vector>
#include <cmath>

bool
TrailRenderer::LoadTrace(const TraceComputer &trace_computer) noexcept
{
  trace.clear();
  trace_computer.LockedCopyTo(trace);
  return !trace.empty();
}

bool
TrailRenderer::LoadTrace(const TraceComputer &trace_computer,
                         TimeStamp min_time,
                         const WindowProjection &projection) noexcept
{
  trace.clear();
  trace_computer.LockedCopyTo(trace,
                              min_time.Cast<std::chrono::duration<unsigned>>(),
                              projection.GetGeoScreenCenter(),
                              projection.DistancePixelsToMeters(3));
  return !trace.empty();
}

/**
 * This function returns the corresponding SnailTrail
 * color array index to the input
 * @param vario Input value between min_vario and max_vario
 * @return SnailTrail color array index
 */
static constexpr unsigned
GetSnailColorIndex(double vario, double min_vario, double max_vario) noexcept
{
  auto cv = vario < 0 ? -vario / min_vario : vario / max_vario;

  return std::clamp((int)((cv + 1) / 2 * TrailLook::NUMSNAILCOLORS),
                    0, (int)(TrailLook::NUMSNAILCOLORS - 1));
}

static constexpr unsigned
GetAltitudeColorIndex(double alt, double min_alt, double max_alt) noexcept
{
  auto relative_altitude = (alt - min_alt) / (max_alt - min_alt);
  int _max = TrailLook::NUMSNAILCOLORS - 1;
  return std::clamp((int)(relative_altitude * _max), 0, _max);
}

[[gnu::pure]]
static std::pair<double, double>
GetMinMax(TrailSettings::Type type, const TracePointVector &trace) noexcept
{
  double value_min, value_max;

  if (type == TrailSettings::Type::ALTITUDE) {
    value_max = 1000;
    value_min = 500;

    for (const auto &i : trace) {
      value_max = std::max(i.GetAltitude(), value_max);
      value_min = std::min(i.GetAltitude(), value_min);
    }
  } else {
    value_max = 0.75;
    value_min = -2.0;

    for (const auto &i : trace) {
      value_max = std::max(i.GetVario(), value_max);
      value_min = std::min(i.GetVario(), value_min);
    }

    value_max = std::min(7.5, value_max);
    value_min = std::max(-5.0, value_min);
  }

  return std::make_pair(value_min, value_max);
}

/**
 * Catmull-Rom spline interpolation between two points.
 * Interpolates between p1 and p2 using p0 and p3 as control points.
 * @param p0 Control point before p1
 * @param p1 Start point
 * @param p2 End point
 * @param p3 Control point after p2
 * @param t Interpolation parameter (0.0 = p1, 1.0 = p2)
 * @return Interpolated point
 */
[[gnu::pure]]
static PixelPoint
CatmullRomInterpolate(const PixelPoint &p0, const PixelPoint &p1,
                      const PixelPoint &p2, const PixelPoint &p3,
                      double t) noexcept
{
  const double t2 = t * t;
  const double t3 = t2 * t;

  // Catmull-Rom spline coefficients
  const double c0 = -0.5 * t3 + t2 - 0.5 * t;
  const double c1 = 1.5 * t3 - 2.5 * t2 + 1.0;
  const double c2 = -1.5 * t3 + 2.0 * t2 + 0.5 * t;
  const double c3 = 0.5 * t3 - 0.5 * t2;

  return PixelPoint(
    static_cast<int>(c0 * p0.x + c1 * p1.x + c2 * p2.x + c3 * p3.x),
    static_cast<int>(c0 * p0.y + c1 * p1.y + c2 * p2.y + c3 * p3.y)
  );
}

/**
 * Calculate the angle between two vectors (in degrees)
 */
[[gnu::pure]]
static double
CalculateAngle(const PixelPoint &p0, const PixelPoint &p1, const PixelPoint &p2) noexcept
{
  const int dx1 = p1.x - p0.x;
  const int dy1 = p1.y - p0.y;
  const int dx2 = p2.x - p1.x;
  const int dy2 = p2.y - p1.y;
  
  const double len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
  const double len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
  
  if (len1 < 1.0 || len2 < 1.0)
    return 0.0;
  
  const double dot = (dx1 * dx2 + dy1 * dy2) / (len1 * len2);
  const double clamped = std::clamp(dot, -1.0, 1.0);
  return std::acos(clamped) * 180.0 / M_PI;
}

/**
 * Generate interpolated points between two trace points using Catmull-Rom spline.
 * Adaptively adjusts the number of segments based on the turn angle for smoother curves.
 * @param p0 Control point before p1
 * @param p1 Start point
 * @param p2 End point
 * @param p3 Control point after p2
 * @param base_num_segments Base number of segments to generate
 * @return Vector of interpolated points (including p1 and p2)
 */
static std::vector<PixelPoint>
InterpolateSegment(const PixelPoint &p0, const PixelPoint &p1,
                   const PixelPoint &p2, const PixelPoint &p3,
                   unsigned base_num_segments) noexcept
{
  // Calculate turn angle to determine if we need more segments
  const double angle = CalculateAngle(p0, p1, p2);
  
  // Sharper turns need more segments for smooth appearance
  // For angles > 30 degrees, increase segments significantly
  unsigned num_segments = base_num_segments;
  if (angle > 45.0) {
    num_segments = base_num_segments * 2;
  } else if (angle > 30.0) {
    num_segments = static_cast<unsigned>(base_num_segments * 1.5);
  } else if (angle > 15.0) {
    num_segments = static_cast<unsigned>(base_num_segments * 1.2);
  }
  
  // Cap maximum segments for performance
  num_segments = std::min(num_segments, 20u);
  
  std::vector<PixelPoint> result;
  result.reserve(num_segments + 1);

  // Always include the start point
  result.push_back(p1);

  // Generate intermediate points
  for (unsigned i = 1; i < num_segments; ++i) {
    const double t = static_cast<double>(i) / num_segments;
    result.push_back(CatmullRomInterpolate(p0, p1, p2, p3, t));
  }

  // Always include the end point
  result.push_back(p2);

  return result;
}

void
TrailRenderer::Draw(Canvas &canvas, const TraceComputer &trace_computer,
                    const WindowProjection &projection,
                    TimeStamp min_time,
                    bool enable_traildrift, const PixelPoint pos,
                    const NMEAInfo &basic, const DerivedInfo &calculated,
                    const TrailSettings &settings) noexcept
{
  if (settings.length == TrailSettings::Length::OFF)
    return;

  if (!LoadTrace(trace_computer, min_time, projection))
    return;

  if (!basic.location_available || !calculated.wind_available)
    enable_traildrift = false;

  GeoPoint traildrift;
  if (enable_traildrift) {
    GeoPoint tp1 = FindLatitudeLongitude(basic.location,
                                         calculated.wind.bearing,
                                         calculated.wind.norm);
    traildrift = basic.location - tp1;
  }

  auto minmax = GetMinMax(settings.type, trace);
  auto value_min = minmax.first;
  auto value_max = minmax.second;

  bool scaled_trail = settings.scaling_enabled &&
                      projection.GetMapScale() <= 6000;

  const GeoBounds bounds = projection.GetScreenBounds().Scale(4);

  // Determine if we should use smoothing (only for line segments, not dots-only modes)
  const bool use_smoothing = 
    settings.type != TrailSettings::Type::VARIO_1_DOTS &&
    settings.type != TrailSettings::Type::VARIO_2_DOTS;

  // Determine number of interpolation segments based on zoom level
  // More segments when zoomed in for better quality, and always use enough for smooth curves
  // Gliders fly in smooth arcs, so we need more segments for realistic appearance
  const unsigned num_segments = projection.GetMapScale() <= 3000 ? 12 : 
                                 projection.GetMapScale() <= 6000 ? 8 : 6;

  // Collect valid points with their data
  struct PointData {
    PixelPoint point;
    double value; // vario or altitude
  };
  std::vector<PointData> valid_points;
  valid_points.reserve(trace.size());

  for (const auto &i : trace) {
    const GeoPoint gp = enable_traildrift
      ? i.GetLocation().Parametric(traildrift, i.CalculateDrift(basic.time))
      : i.GetLocation();
    if (!bounds.IsInside(gp)) {
      continue;
    }

    auto pt = projection.GeoToScreen(gp);
    const double value = (settings.type == TrailSettings::Type::ALTITUDE)
      ? i.GetAltitude() : i.GetVario();

    valid_points.push_back({pt, value});
  }

  if (valid_points.empty())
    return;

  // Draw the trail with spline interpolation
  for (size_t i = 0; i < valid_points.size(); ++i) {
    if (i == 0) {
      // First point - just store it
      continue;
    }

    const auto &prev_data = valid_points[i - 1];
    const auto &curr_data = valid_points[i];

    // Determine color index for this segment
    unsigned color_index;
    if (settings.type == TrailSettings::Type::ALTITUDE) {
      color_index = GetAltitudeColorIndex(curr_data.value,
                                          value_min, value_max);
    } else {
      color_index = GetSnailColorIndex(curr_data.value,
                                       value_min, value_max);
    }

    // Determine if we should draw dots (circles) for this segment
    // Dots are drawn for negative vario in dots modes, or for positive vario in VARIO_DOTS_AND_LINES/VARIO_EINK
    const bool draw_dots = 
      (curr_data.value < 0 &&
       (settings.type == TrailSettings::Type::VARIO_1_DOTS ||
        settings.type == TrailSettings::Type::VARIO_2_DOTS ||
        settings.type == TrailSettings::Type::VARIO_DOTS_AND_LINES ||
        settings.type == TrailSettings::Type::VARIO_EINK)) ||
      (curr_data.value >= 0 &&
       (settings.type == TrailSettings::Type::VARIO_DOTS_AND_LINES ||
        settings.type == TrailSettings::Type::VARIO_EINK));

    // Determine if we should draw lines for this segment
    // Lines are NOT drawn only for negative vario in VARIO_1_DOTS/VARIO_2_DOTS modes
    const bool draw_lines = 
      !(curr_data.value < 0 &&
        (settings.type == TrailSettings::Type::VARIO_1_DOTS ||
         settings.type == TrailSettings::Type::VARIO_2_DOTS));

    // Draw dots if needed
    if (draw_dots) {
      if (curr_data.value < 0) {
        // Negative vario: draw filled circle with no pen
        canvas.SelectNullPen();
        canvas.Select(look.trail_brushes[color_index]);
      } else {
        // Positive vario in VARIO_DOTS_AND_LINES/VARIO_EINK: draw circle with pen
        canvas.Select(look.trail_brushes[color_index]);
        canvas.Select(look.trail_pens[color_index]);
      }
      canvas.DrawCircle({(curr_data.point.x + prev_data.point.x) / 2,
                         (curr_data.point.y + prev_data.point.y) / 2},
                        look.trail_widths[color_index]);
    }

    // Draw lines if needed
    if (draw_lines) {
      // Determine control points for spline interpolation
      // For beginning/end, duplicate points to create smooth curves
      const auto &p0 = (i >= 2) ? valid_points[i - 2].point : prev_data.point;
      const auto &p1 = prev_data.point;
      const auto &p2 = curr_data.point;
      const auto &p3 = (i + 1 < valid_points.size()) ? valid_points[i + 1].point : curr_data.point;

      if (use_smoothing && i >= 1) {
        // Use spline interpolation for smooth curves
        auto interpolated = InterpolateSegment(p0, p1, p2, p3, num_segments);
        
        // Interpolate values for smooth color transitions
        for (size_t j = 0; j + 1 < interpolated.size(); ++j) {
          const double t = static_cast<double>(j + 1) / interpolated.size();
          const double interp_value = prev_data.value * (1.0 - t) + curr_data.value * t;
          
          unsigned seg_color_index;
          if (settings.type == TrailSettings::Type::ALTITUDE) {
            seg_color_index = GetAltitudeColorIndex(interp_value, value_min, value_max);
          } else {
            seg_color_index = GetSnailColorIndex(interp_value, value_min, value_max);
          }


          if (scaled_trail)
            canvas.Select(look.scaled_trail_pens[seg_color_index]);
          else
            canvas.Select(look.trail_pens[seg_color_index]);

          canvas.DrawLinePiece(interpolated[j], interpolated[j + 1]);
        }
      } else {
        // Not enough points or smoothing disabled - draw direct line

        if (scaled_trail)
          canvas.Select(look.scaled_trail_pens[color_index]);
        else
          canvas.Select(look.trail_pens[color_index]);
        canvas.DrawLinePiece(prev_data.point, curr_data.point);
      }
    }
  }

  // Draw line to current aircraft position
  if (!valid_points.empty()) {
    const auto &last_data = valid_points.back();
    unsigned color_index;
    if (settings.type == TrailSettings::Type::ALTITUDE) {
      color_index = GetAltitudeColorIndex(last_data.value,
                                         value_min, value_max);
    } else {
      color_index = GetSnailColorIndex(last_data.value,
                                      value_min, value_max);
    }

    if (scaled_trail)
      canvas.Select(look.scaled_trail_pens[color_index]);
    else
      canvas.Select(look.trail_pens[color_index]);
    canvas.DrawLine(last_data.point, pos);
  }
}

void
TrailRenderer::Draw(Canvas &canvas, const WindowProjection &projection) noexcept
{
  canvas.Select(look.trace_pen);
  DrawTraceVector(canvas, projection, trace);
}

void
TrailRenderer::Draw(Canvas &canvas, const TraceComputer &trace_computer,
                    const WindowProjection &projection,
                    TimeStamp min_time) noexcept
{
  if (LoadTrace(trace_computer, min_time, projection))
    Draw(canvas, projection);
}

BulkPixelPoint *
TrailRenderer::Prepare(unsigned n) noexcept
{
  points.GrowDiscard(n);
  return points.data();
}

void
TrailRenderer::DrawPreparedPolyline(Canvas &canvas, unsigned n) noexcept
{
  assert(points.size() >= n);

  canvas.DrawPolyline(points.data(), n);
}

void
TrailRenderer::DrawPreparedPolygon(Canvas &canvas, unsigned n) noexcept
{
  assert(points.size() >= n);

  canvas.DrawPolygon(points.data(), n);
}

void
TrailRenderer::DrawTraceVector(Canvas &canvas, const Projection &projection,
                               const ContestTraceVector &trace) noexcept
{
  const unsigned n = trace.size();

  std::transform(trace.begin(), trace.end(), Prepare(n), [&projection](const auto &i){
    return projection.GeoToScreen(i.GetLocation());
  });

  DrawPreparedPolyline(canvas, n);
}

void
TrailRenderer::DrawTriangle(Canvas &canvas, const Projection &projection,
                            const ContestTraceVector &trace) noexcept
{
  assert(trace.size() == 5);

  const unsigned start = 1, n = 3;

  auto *p = Prepare(n);

  for (unsigned i = start; i < start + n; ++i)
    *p++ = projection.GeoToScreen(trace[i].GetLocation());

  DrawPreparedPolygon(canvas, n);
}

void
TrailRenderer::DrawTraceVector(Canvas &canvas, const Projection &projection,
                               const TracePointVector &trace) noexcept
{
  const unsigned n = trace.size();

  std::transform(trace.begin(), trace.end(), Prepare(n), [&projection](const auto &i){
    return projection.GeoToScreen(i.GetLocation());
  });

  DrawPreparedPolyline(canvas, n);
}
