// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrailRenderer.hpp"
#include "Asset.hpp"
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
#include "Math/Point2D.hpp"

#include <algorithm>
#include <new>
#include <utility>
#include <vector>
#include <cmath>

static constexpr double TRAIL_ZOOMED_IN_SCALE = 6000;
static constexpr int TRAIL_THIN_PIXELS_MIN = 3;
static constexpr int TRAIL_THIN_PIXELS_MAX = 24;

namespace {

/**
 * Merge-vario samples in the half-open GPS interval [t0, t1).
 * \a search_from advances monotonically across successive segments.
 */
struct MergeSampleRange {
  size_t begin{};
  size_t end{}; /* one past the last sample in range */
};

static MergeSampleRange
FindMergeSamplesBetween(TracePoint::Time t0, TracePoint::Time t1,
                        const std::vector<TrailVarioSample> &samples,
                        size_t &search_from) noexcept
{
  while (search_from < samples.size() && samples[search_from].time < t0)
    ++search_from;

  const size_t begin = search_from;
  while (search_from < samples.size() && samples[search_from].time < t1)
    ++search_from;

  return {begin, search_from};
}

void
BuildVarioBreakpoints(TracePoint::Time t0, double v0,
                      TracePoint::Time t1, double v1,
                      const std::vector<TrailVarioSample> &samples,
                      size_t &merge_sample_index,
                      std::vector<std::pair<double, double>> &bps) noexcept
{
  bps.clear();
  if (!(t1 > t0))
    return;

  const MergeSampleRange range =
    FindMergeSamplesBetween(t0, t1, samples, merge_sample_index);

  const size_t n = range.end - range.begin;
  bps.reserve(n + 2);
  bps.emplace_back(0., v0);
  if (n == 0) {
    bps.emplace_back(1., v1);
    return;
  }

  const double dt = (double)(t1 - t0).count();

  auto sample_u = [&](size_t i) noexcept -> double {
    if (dt > 0.)
      return (double)(samples[range.begin + i].time - t0).count() / dt;
    return (double)(i + 1) / (double)(n + 1);
  };

  double u_prev = 0.;
  for (size_t i = 0; i < n; ) {
    const double u_i = sample_u(i);
    size_t j = i + 1;
    while (j < n && sample_u(j) == u_i)
      ++j;

    const double u_right = (j < n) ? sample_u(j) : 1.;
    const size_t m = j - i;
    const double u_left = u_prev;
    for (size_t k = i; k < j; ++k) {
      const double u = (m > 1)
        ? u_left + (u_right - u_left) * (double)(k - i + 1) / (double)(m + 1)
        : u_i;
      bps.emplace_back(u, (double)samples[range.begin + k].vario);
      u_prev = u;
    }
    i = j;
  }

  bps.emplace_back(1., v1);
}

[[gnu::pure]]
double
LookupVarioAtU(double u,
               const std::vector<std::pair<double, double>> &bps) noexcept
{
  if (bps.empty())
    return 0;
  if (u <= bps.front().first)
    return bps.front().second;
  if (u >= bps.back().first)
    return bps.back().second;

  const auto it = std::upper_bound(bps.begin(), bps.end(), u,
                                   [](double u, const auto &bp) {
                                     return u < bp.first;
                                   });
  const auto &b = *it;
  const auto &a = *std::prev(it);
  const double du = b.first - a.first;
  if (du <= 0.)
    return b.second;
  const double t = (u - a.first) / du;
  return a.second * (1. - t) + b.second * t;
}

/** ~1 canvas pixel per 500 m map scale, clamped to pre/post #2661 range. */
[[gnu::const]]
static double
GetTrailThinDistance(const WindowProjection &projection,
                     double map_scale) noexcept
{
  const int thin_px = std::clamp(int(map_scale / 500),
                                 TRAIL_THIN_PIXELS_MIN,
                                 TRAIL_THIN_PIXELS_MAX);
  return projection.DistancePixelsToMeters(thin_px);
}

[[gnu::pure]]
static PixelPoint
LerpPixelPoint(const PixelPoint &a, const PixelPoint &b,
               double u) noexcept
{
  return PixelPoint(
    int(std::lround(a.x + (b.x - a.x) * u)),
    int(std::lround(a.y + (b.y - a.y) * u)));
}

[[gnu::const]]
static unsigned
GetSplineBaseSegments(const PixelPoint &p1, const PixelPoint &p2) noexcept
{
  unsigned base = unsigned(std::clamp(ManhattanDistance(p1, p2) / 4, 4, 16));
  if (IsEmbedded() && base > 4)
    base = base * 3 / 4;
  return std::max(base, 4u);
}

static void
BuildDirectSegmentPoints(const PixelPoint &p1, const PixelPoint &p2,
                         const std::vector<std::pair<double, double>> &bps,
                         bool use_merge_vario,
                         std::vector<PixelPoint> &result) noexcept
{
  result.clear();
  result.push_back(p1);

  if (use_merge_vario && bps.size() > 2) {
    const unsigned pieces = unsigned(bps.size() - 1);
    for (unsigned j = 1; j < pieces; ++j)
      result.push_back(LerpPixelPoint(p1, p2, double(j) / double(pieces)));
  }

  result.push_back(p2);
}

} // namespace

bool
TrailRenderer::LoadTrace(const TraceComputer &trace_computer) noexcept
{
  trace.clear();
  merge_vario_samples.clear();
  try {
    trace_computer.LockedCopySnapshot(trace, merge_vario_samples);
  } catch (const std::bad_alloc &) {
    trace.clear();
    merge_vario_samples.clear();
    return false;
  }
  return !trace.empty();
}

bool
TrailRenderer::LoadTrace(const TraceComputer &trace_computer,
                         TimeStamp min_time,
                         const WindowProjection &projection,
                         double map_scale) noexcept
{
  trace.clear();
  merge_vario_samples.clear();
  try {
    trace_computer.LockedCopySnapshot(trace, merge_vario_samples,
                                      min_time.Cast<std::chrono::duration<unsigned>>(),
                                      projection.GetGeoScreenCenter(),
                                      GetTrailThinDistance(projection,
                                                           map_scale));
  } catch (const std::bad_alloc &) {
    trace.clear();
    merge_vario_samples.clear();
    return false;
  }
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
static unsigned
GetTrailColorIndex(TrailSettings::Type type, double value,
                   double value_min, double value_max) noexcept
{
  if (type == TrailSettings::Type::ALTITUDE)
    return GetAltitudeColorIndex(value, value_min, value_max);
  return GetSnailColorIndex(value, value_min, value_max);
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
 * Turn angle at \a p1 between segments p0–p1 and p1–p2 (degrees).
 */
[[gnu::pure]]
static double
CalculateAngle(const PixelPoint &p0, const PixelPoint &p1, const PixelPoint &p2) noexcept
{
  const IntPoint2D v1 = p1 - p0;
  const IntPoint2D v2 = p2 - p1;

  const double len1 = v1.Magnitude();
  const double len2 = v2.Magnitude();

  if (len1 < 1.0 || len2 < 1.0)
    return 0.0;

  const double dot = DotProduct(v1, v2) / (len1 * len2);
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
 * @param result Reused output buffer (including p1 and p2)
 */
static void
InterpolateSegment(const PixelPoint &p0, const PixelPoint &p1,
                   const PixelPoint &p2, const PixelPoint &p3,
                   unsigned base_num_segments, unsigned max_segments,
                   std::vector<PixelPoint> &result) noexcept
{
  const double angle = CalculateAngle(p0, p1, p2);

  unsigned num_segments = base_num_segments;
  if (angle > 45.0) {
    num_segments = base_num_segments * 2;
  } else if (angle > 30.0) {
    num_segments = static_cast<unsigned>(base_num_segments * 1.5);
  } else if (angle > 15.0) {
    num_segments = static_cast<unsigned>(base_num_segments * 1.2);
  }

  num_segments = std::min(num_segments, max_segments);

  result.clear();
  result.reserve(num_segments + 1);
  result.push_back(p1);

  for (unsigned i = 1; i < num_segments; ++i) {
    const double t = static_cast<double>(i) / num_segments;
    result.push_back(CatmullRomInterpolate(p0, p1, p2, p3, t));
  }

  result.push_back(p2);
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

  const double map_scale = projection.GetMapScale();

  if (!LoadTrace(trace_computer, min_time, projection, map_scale))
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

  const bool zoomed_in = map_scale <= TRAIL_ZOOMED_IN_SCALE;
  const bool scaled_trail = settings.scaling_enabled && zoomed_in;

  const GeoBounds bounds = projection.GetScreenBounds().Scale(4);

  const bool use_smoothing =
    settings.type != TrailSettings::Type::VARIO_1_DOTS &&
    settings.type != TrailSettings::Type::VARIO_2_DOTS &&
    zoomed_in;

  valid_points.clear();
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
    const TrailPointData pd{pt, value, i.GetTime()};

    if (!valid_points.empty() &&
        ManhattanDistance(pt, valid_points.back().point) < 1) {
      /* Same screen cell: keep latest vario/altitude for colour */
      valid_points.back() = pd;
      continue;
    }

    valid_points.push_back(pd);
  }

  if (valid_points.empty())
    return;

  size_t merge_sample_index = 0;

  // Draw the trail with spline interpolation
  for (size_t i = 0; i < valid_points.size(); ++i) {
    if (i == 0) {
      // First point - just store it
      continue;
    }

    const auto &prev_data = valid_points[i - 1];
    const auto &curr_data = valid_points[i];

    const unsigned color_index =
      GetTrailColorIndex(settings.type, curr_data.value,
                         value_min, value_max);

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

      const bool use_merge_vario =
        settings.type != TrailSettings::Type::ALTITUDE &&
        !merge_vario_samples.empty();

      if (use_merge_vario)
        BuildVarioBreakpoints(prev_data.time, prev_data.value,
                              curr_data.time, curr_data.value,
                              merge_vario_samples, merge_sample_index,
                              vario_breakpoints);
      else
        vario_breakpoints.clear();

      if (use_smoothing && i >= 1) {
        const unsigned base_segments = GetSplineBaseSegments(p1, p2);
        InterpolateSegment(p0, p1, p2, p3, base_segments, 16u,
                           interpolated);
        DrawSegmentWithVarioColour(canvas, prev_data, curr_data,
                                   interpolated, settings.type,
                                   value_min, value_max,
                                   scaled_trail, use_merge_vario);
      } else {
        BuildDirectSegmentPoints(p1, p2, vario_breakpoints,
                                 use_merge_vario, interpolated);
        DrawSegmentWithVarioColour(canvas, prev_data, curr_data,
                                   interpolated, settings.type,
                                   value_min, value_max,
                                   scaled_trail, use_merge_vario);
      }
    }
  }

  // Draw line to current aircraft position
  if (!valid_points.empty()) {
    const auto &last_data = valid_points.back();
    const unsigned color_index =
      GetTrailColorIndex(settings.type, last_data.value,
                         value_min, value_max);

    SelectTrailPen(canvas, color_index, scaled_trail);
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
  if (LoadTrace(trace_computer, min_time, projection,
                projection.GetMapScale()))
    Draw(canvas, projection);
}

BulkPixelPoint *
TrailRenderer::Prepare(unsigned n) noexcept
{
  points.GrowDiscard(n);
  return points.data();
}

void
TrailRenderer::SelectTrailPen(Canvas &canvas, unsigned color_index,
                              bool scaled_trail) const noexcept
{
  if (scaled_trail)
    canvas.Select(look.scaled_trail_pens[color_index]);
  else
    canvas.Select(look.trail_pens[color_index]);
}

void
TrailRenderer::PrepareCopy(const PixelPoint *src, unsigned n) noexcept
{
  std::copy_n(src, n, Prepare(n));
}

void
TrailRenderer::DrawColourPolyline(Canvas &canvas, unsigned color_index,
                                   bool scaled_trail,
                                   const std::vector<PixelPoint> &pts,
                                   size_t first, size_t last) noexcept
{
  assert(first <= last);
  assert(last < pts.size());

  const unsigned n = unsigned(last - first + 1);
  if (n < 2)
    return;

  PrepareCopy(pts.data() + first, n);
  SelectTrailPen(canvas, color_index, scaled_trail);
  DrawPreparedPolyline(canvas, n);
}

void
TrailRenderer::DrawSegmentWithVarioColour(
    Canvas &canvas,
    const TrailPointData &prev_data,
    const TrailPointData &curr_data,
    const std::vector<PixelPoint> &segment_pts,
    TrailSettings::Type type,
    double value_min, double value_max,
    bool scaled_trail,
    bool use_merge_vario) noexcept
{
  if (segment_pts.size() < 2)
    return;

  const double piece_count =
    segment_pts.size() > 1
      ? static_cast<double>(segment_pts.size() - 1)
      : 1.0;

  size_t run_start = 0;
  unsigned run_color = 0;
  bool run_active = false;

  auto flush_colour_run = [&](size_t run_end) {
    if (!run_active || run_end <= run_start)
      return;
    DrawColourPolyline(canvas, run_color, scaled_trail,
                       segment_pts, run_start, run_end);
  };

  for (size_t j = 0; j + 1 < segment_pts.size(); ++j) {
    double interp_value;
    if (type == TrailSettings::Type::ALTITUDE) {
      const double t = static_cast<double>(j + 1) / piece_count;
      interp_value =
        prev_data.value * (1.0 - t) + curr_data.value * t;
    } else if (use_merge_vario && !vario_breakpoints.empty()) {
      const double u = static_cast<double>(j + 0.5) / piece_count;
      interp_value = LookupVarioAtU(u, vario_breakpoints);
    } else {
      const double t = static_cast<double>(j + 1) / piece_count;
      interp_value =
        prev_data.value * (1.0 - t) + curr_data.value * t;
    }

    const unsigned seg_color_index =
      GetTrailColorIndex(type, interp_value, value_min, value_max);

    if (!run_active) {
      run_start = j;
      run_color = seg_color_index;
      run_active = true;
    } else if (seg_color_index != run_color) {
      flush_colour_run(j);
      run_start = j;
      run_color = seg_color_index;
    }
  }

  if (run_active)
    flush_colour_run(segment_pts.size() - 1);
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
