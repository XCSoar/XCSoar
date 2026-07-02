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
#include "Geo/GeoBounds.hpp"
#include "Engine/Contest/ContestTrace.hpp"

#include <algorithm>
#include <new>
#include <utility>
#include <vector>
#include <cmath>

static constexpr double TRAIL_ZOOMED_OUT_MAP_SCALE = 6000;
static constexpr int TRAIL_SPACING_PX = 6;
static constexpr int TRAIL_THIN_PIXELS_MIN = 3;
static constexpr int TRAIL_THIN_PIXELS_MAX = 24;
static constexpr double TRAIL_BOUNDS_SCALE = 4.;
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

[[gnu::pure]]
static size_t
FindMergeSampleIndexAtOrAfter(
    TracePoint::Time time,
    const std::vector<TrailVarioSample> &samples) noexcept
{
  return std::lower_bound(samples.begin(), samples.end(), time,
                          [](const TrailVarioSample &sample,
                             TracePoint::Time t) noexcept {
                            return sample.time < t;
                          }) - samples.begin();
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

[[gnu::const]]
static bool
IsVarioDotsOnlyMode(TrailSettings::Type type) noexcept
{
  return type == TrailSettings::Type::VARIO_1_DOTS ||
    type == TrailSettings::Type::VARIO_2_DOTS;
}

[[gnu::pure]]
static double
InterpolatePieceValue(TrailSettings::Type type,
                      double prev_value,
                      double curr_value,
                      unsigned piece_index,
                      double piece_count,
                      bool use_merge_vario,
                      const std::vector<std::pair<double, double>> &bps) noexcept
{
  if (type == TrailSettings::Type::ALTITUDE) {
    const double t = (piece_index + 1) / piece_count;
    return prev_value * (1.0 - t) + curr_value * t;
  }

  if (use_merge_vario && !bps.empty()) {
    const double u = (piece_index + 0.5) / piece_count;
    return LookupVarioAtU(u, bps);
  }

  const double t = (piece_index + 1) / piece_count;
  return prev_value * (1.0 - t) + curr_value * t;
}

struct CatmullRomWeights {
  double c0, c1, c2, c3;
};

[[gnu::const]]
static CatmullRomWeights
ComputeCatmullRomWeights(double t) noexcept
{
  const double t2 = t * t;
  const double t3 = t2 * t;
  return {
    -0.5 * t3 + t2 - 0.5 * t,
    1.5 * t3 - 2.5 * t2 + 1.0,
    -1.5 * t3 + 2.0 * t2 + 0.5 * t,
    0.5 * t3 - 0.5 * t2,
  };
}

/** Target screen-pixel spacing between kept trace fixes. */
[[gnu::const]]
static int
GetTrailSpacingPixels() noexcept
{
  return std::clamp(TRAIL_SPACING_PX,
                    TRAIL_THIN_PIXELS_MIN,
                    TRAIL_THIN_PIXELS_MAX);
}

[[gnu::const]]
static double
GetTrailThinDistance(const WindowProjection &projection) noexcept
{
  return projection.DistancePixelsToMeters(GetTrailSpacingPixels());
}

/**
 * Bounded Catmull-Rom smoothing budget and recent trail tail.
 * Stefan Schumann, PR #2664.
 */
static constexpr size_t MAX_SMOOTHED_TRAIL_POINTS = 180;

[[gnu::const]]
static unsigned
GetBaseSmoothingSegments(const WindowProjection &projection) noexcept
{
  const double map_scale = projection.GetMapScale();
  return map_scale <= 3000 ? 4 : map_scale <= TRAIL_ZOOMED_OUT_MAP_SCALE ? 3 : 2;
}

[[gnu::const]]
static unsigned
GetPreferredSmoothingSegments(const WindowProjection &projection) noexcept
{
  const double map_scale = projection.GetMapScale();
  return map_scale <= 3000 ? 8 : map_scale <= TRAIL_ZOOMED_OUT_MAP_SCALE ? 6 : 4;
}

[[gnu::const]]
static unsigned
GetSmoothingSegments(const WindowProjection &projection,
                     size_t point_count) noexcept
{
  const unsigned base = GetBaseSmoothingSegments(projection);
  if (point_count <= 1)
    return base;

  const unsigned budget = (MAX_SMOOTHED_TRAIL_POINTS - 1) * base;
  const unsigned allowed = std::max(base,
                                    budget / (unsigned)(point_count - 1));
  return std::min(GetPreferredSmoothingSegments(projection), allowed);
}

[[gnu::const]]
static size_t
GetFirstSmoothedPointIndex(size_t point_count) noexcept
{
  return point_count > MAX_SMOOTHED_TRAIL_POINTS
    ? point_count - MAX_SMOOTHED_TRAIL_POINTS
    : 0;
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

/**
 * Catmull-Rom spline interpolation between two points.
 */
[[gnu::pure]]
static PixelPoint
CatmullRomInterpolate(const PixelPoint &p0, const PixelPoint &p1,
                      const PixelPoint &p2, const PixelPoint &p3,
                      double t) noexcept
{
  const CatmullRomWeights c = ComputeCatmullRomWeights(t);

  return PixelPoint(
    static_cast<int>(c.c0 * p0.x + c.c1 * p1.x + c.c2 * p2.x + c.c3 * p3.x),
    static_cast<int>(c.c0 * p0.y + c.c1 * p1.y + c.c2 * p2.y + c.c3 * p3.y)
  );
}

[[gnu::pure]]
static GeoPoint
TrailGeoPoint(const TracePoint &tp, bool enable_traildrift,
              const GeoPoint &traildrift,
              const NMEAInfo &basic) noexcept
{
  if (!enable_traildrift)
    return tp.GetLocation();

  return tp.GetLocation().Parametric(traildrift,
                                     tp.CalculateDrift(basic.time));
}

[[gnu::pure]]
static GeoPoint
CatmullRomGeo(const GeoPoint &p0, const GeoPoint &p1,
              const GeoPoint &p2, const GeoPoint &p3,
              double t) noexcept
{
  const CatmullRomWeights c = ComputeCatmullRomWeights(t);

  return GeoPoint(
    Angle::Native(c.c0 * p0.longitude.Native() + c.c1 * p1.longitude.Native() +
                  c.c2 * p2.longitude.Native() + c.c3 * p3.longitude.Native()),
    Angle::Native(c.c0 * p0.latitude.Native() + c.c1 * p1.latitude.Native() +
                  c.c2 * p2.latitude.Native() + c.c3 * p3.latitude.Native()));
}

[[gnu::pure]]
static GeoPoint
DriftGeoPoint(const GeoPoint &geo, TracePoint::Time time,
              uint16_t drift_factor,
              bool enable_traildrift,
              const GeoPoint &traildrift,
              TimeStamp now) noexcept
{
  if (!enable_traildrift)
    return geo;

  const double dt =
    (now.ToDuration() - std::chrono::duration_cast<FloatDuration>(time)).count();
  return geo.Parametric(traildrift, dt * drift_factor / 256);
}

} // namespace

void
TrailRenderer::MergeAdjacentColourRuns(
    std::vector<CachedColourRun> &runs) noexcept
{
  if (runs.size() < 2)
    return;

  size_t write = 0;
  for (size_t read = 1; read < runs.size(); ++read) {
    if (runs[read].color_index == runs[write].color_index) {
      const auto &src = runs[read].points;
      if (src.empty())
        continue;

      size_t start = 0;
      if (!runs[write].points.empty() &&
          runs[write].points.back().geo == src.front().geo)
        start = 1;

      runs[write].points.insert(runs[write].points.end(),
                                src.begin() + start, src.end());
    } else {
      ++write;
      if (write != read)
        runs[write] = std::move(runs[read]);
    }
  }
  runs.resize(write + 1);
}

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
                         const WindowProjection &projection) noexcept
{
  const TrailQuery query = MakeTrailQuery(min_time, projection);
  return SyncTrace(trace_computer, query);
}

TrailQuery
TrailRenderer::MakeTrailQuery(TimeStamp min_time,
                              const WindowProjection &projection) noexcept
{
  TrailQuery query;
  query.min_time = min_time.Cast<std::chrono::duration<unsigned>>();
  query.bounds = projection.GetScreenBounds().Scale(TRAIL_BOUNDS_SCALE);
  query.project_location = projection.GetGeoScreenCenter();
  query.min_distance_m = GetTrailThinDistance(projection);
  return query;
}

bool
TrailRenderer::TrailDrawFingerprint::operator==(
    const TrailDrawFingerprint &other) const noexcept
{
  return scale_px_per_m == other.scale_px_per_m &&
    color_min == other.color_min &&
    color_max == other.color_max &&
    query_bounds.GetNorthWest() == other.query_bounds.GetNorthWest() &&
    query_bounds.GetSouthEast() == other.query_bounds.GetSouthEast();
}

void
TrailRenderer::InvalidateSegmentCache() noexcept
{
  segment_cache.clear();
  cached_trace_size = 0;
}

bool
TrailRenderer::SyncTrace(const TraceComputer &trace_computer,
                         const TrailQuery &query) noexcept
{
  Serial append{}, modify{};
  trace.clear();
  merge_vario_samples.clear();
  try {
    trace_computer.LockedTrailQuery(query, trace, merge_vario_samples,
                                    &append, &modify);
  } catch (const std::bad_alloc &) {
    trace.clear();
    merge_vario_samples.clear();
    InvalidateSegmentCache();
    return false;
  }

  synced_append_serial = append;
  synced_modify_serial = modify;
  return !trace.empty();
}

unsigned
TrailRenderer::ColorScale::Index(double value) const noexcept
{
  static constexpr unsigned max_index = TrailLook::NUMSNAILCOLORS - 1;

  if (is_altitude) {
    const int idx = int((value - alt_min) * alt_inv_range);
    if (idx <= 0)
      return 0;
    if (unsigned(idx) >= max_index)
      return max_index;
    return unsigned(idx);
  }

  const double cv = value < 0 ? value * neg_inv_min : value * inv_max;
  const int idx =
    int((cv + 1) * (TrailLook::NUMSNAILCOLORS * 0.5));
  if (idx <= 0)
    return 0;
  if (unsigned(idx) >= max_index)
    return max_index;
  return unsigned(idx);
}

TrailRenderer::ColorScale
TrailRenderer::ColorScale::FromMinMax(TrailSettings::Type type,
                                      double value_min,
                                      double value_max) noexcept
{
  ColorScale scale;
  if (type == TrailSettings::Type::ALTITUDE) {
    scale.is_altitude = true;
    scale.alt_min = value_min;
    const double range = value_max - value_min;
    scale.alt_inv_range = range > 0
      ? double(TrailLook::NUMSNAILCOLORS - 1) / range
      : 0.;
  } else {
    scale.neg_inv_min = -1.0 / value_min;
    scale.inv_max = 1.0 / value_max;
  }
  return scale;
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

void
TrailRenderer::DrawCachedSegments(Canvas &canvas,
                                  const WindowProjection &projection,
                                  const TrailSettings::Type type,
                                  const bool scaled_trail,
                                  const bool enable_traildrift,
                                  const GeoPoint &traildrift,
                                  const NMEAInfo &basic,
                                  const std::vector<CachedTrailSegment> &segments) noexcept
{
  const bool suppress_sink_lines = IsVarioDotsOnlyMode(type);
  static constexpr unsigned null_color_index =
    TrailLook::NUMSNAILCOLORS / 2;

  size_t max_batch_points = 0;
  size_t current_batch_points = 0;
  unsigned current_batch_color = TrailLook::NUMSNAILCOLORS;
  for (const auto &seg : segments) {
    for (const auto &run : seg.colour_runs) {
      if (run.points.size() < 2)
        continue;

      if (suppress_sink_lines && run.color_index < null_color_index)
        continue;

      if (run.color_index != current_batch_color) {
        max_batch_points = std::max(max_batch_points, current_batch_points);
        current_batch_points = 0;
        current_batch_color = run.color_index;
      }

      current_batch_points += run.points.size();
    }
  }

  max_batch_points = std::max(max_batch_points, current_batch_points);
  if (max_batch_points > 0)
    points.GrowDiscard(max_batch_points);

  unsigned batch_color = TrailLook::NUMSNAILCOLORS;
  unsigned batch_n = 0;

  auto flush_batch = [&]() noexcept {
    if (batch_n < 2)
      return;

    SelectTrailPen(canvas, batch_color, scaled_trail);
    DrawPreparedPolyline(canvas, batch_n);
    batch_n = 0;
  };

  for (const auto &seg : segments) {
    for (const auto &run : seg.colour_runs) {
      if (run.points.size() < 2)
        continue;

      if (suppress_sink_lines && run.color_index < null_color_index)
        continue;

      if (run.color_index != batch_color)
        flush_batch();

      batch_color = run.color_index;

      size_t start = 0;
      if (batch_n > 0 && !run.points.empty()) {
        const auto &junction_pt = run.points.front();
        const PixelPoint junction = projection.GeoToScreen(
          DriftGeoPoint(junction_pt.geo, junction_pt.time,
                        junction_pt.drift_factor,
                        enable_traildrift, traildrift, basic.time));
        if (junction.x == points[batch_n - 1].x &&
            junction.y == points[batch_n - 1].y)
          start = 1;
      }

      for (size_t i = start; i < run.points.size(); ++i) {
        const auto &p = run.points[i];
        const PixelPoint pt = projection.GeoToScreen(
          DriftGeoPoint(p.geo, p.time, p.drift_factor,
                        enable_traildrift, traildrift, basic.time));

        points[batch_n++] = pt;
      }
    }
  }

  flush_batch();
}

void
TrailRenderer::AppendColourRun(std::vector<CachedColourRun> &runs,
                               const unsigned color_index,
                               const CachedPathPoint &pt) noexcept
{
  if (!runs.empty() && runs.back().color_index == color_index) {
    runs.back().points.push_back(pt);
    return;
  }

  std::vector<CachedPathPoint> pts;
  if (!runs.empty())
    pts.push_back(runs.back().points.back());
  pts.push_back(pt);
  runs.push_back({color_index, std::move(pts)});
}

TrailRenderer::CachedPathPoint
TrailRenderer::LerpCachedPathPoint(const GeoPoint &g0, const GeoPoint &g1,
                                   const TracePoint::Time t0,
                                   const TracePoint::Time t1,
                                   const unsigned df0, const unsigned df1,
                                   const double u) noexcept
{
  return {
    g0.Interpolate(g1, u),
    TracePoint::Time((unsigned)(t0.count() * (1. - u) + t1.count() * u)),
    (uint16_t)(df0 * (1. - u) + df1 * u),
  };
}

void
TrailRenderer::BuildDirectColourRuns(
    const TrailPointData &prev_data,
    const TrailPointData &curr_data,
    const GeoPoint &prev_geo,
    const GeoPoint &curr_geo,
    const unsigned prev_drift_factor,
    const unsigned curr_drift_factor,
    const std::vector<PixelPoint> &segment_pts,
    TrailSettings::Type type,
    const ColorScale &color_scale,
    bool use_merge_vario,
    std::vector<CachedColourRun> &runs) noexcept
{
  if (segment_pts.size() < 2)
    return;

  const double piece_count =
    segment_pts.size() > 1
      ? static_cast<double>(segment_pts.size() - 1)
      : 1.0;

  for (size_t j = 0; j + 1 < segment_pts.size(); ++j) {
    const double u = static_cast<double>(j) / piece_count;
    const double interp_value =
      InterpolatePieceValue(type, prev_data.value, curr_data.value,
                            unsigned(j), piece_count, use_merge_vario,
                            vario_breakpoints);

    AppendColourRun(runs, color_scale.Index(interp_value),
                    LerpCachedPathPoint(prev_geo, curr_geo,
                                        prev_data.time, curr_data.time,
                                        prev_drift_factor, curr_drift_factor,
                                        u));
  }

  AppendColourRun(runs, color_scale.Index(curr_data.value),
                  {curr_geo, curr_data.time,
                   (uint16_t)curr_drift_factor});
}

void
TrailRenderer::BuildSmoothColourRuns(
    const GeoPoint &g0, const GeoPoint &g1,
    const GeoPoint &g2, const GeoPoint &g3,
    const TracePoint::Time time1,
    const TracePoint::Time time2,
    const unsigned drift_factor1,
    const unsigned drift_factor2,
    const unsigned num_segments,
    const TrailPointData &prev_data,
    const TrailPointData &curr_data,
    TrailSettings::Type type,
    const ColorScale &color_scale,
    bool use_merge_vario,
    std::vector<CachedColourRun> &runs) noexcept
{
  const double piece_count = static_cast<double>(num_segments);

  AppendColourRun(runs, color_scale.Index(prev_data.value),
                  {g1, time1, (uint16_t)drift_factor1});

  for (unsigned j = 0; j < num_segments; ++j) {
    const unsigned next_piece = j + 1;
    const double t = static_cast<double>(next_piece) / piece_count;
    const GeoPoint next_geo =
      next_piece == num_segments
        ? g2
        : CatmullRomGeo(g0, g1, g2, g3, t);

    const double interp_value =
      InterpolatePieceValue(type, prev_data.value, curr_data.value,
                            j, piece_count, use_merge_vario,
                            vario_breakpoints);

    const CachedPathPoint meta =
      LerpCachedPathPoint(g1, g2, time1, time2,
                          drift_factor1, drift_factor2, t);

    AppendColourRun(runs, color_scale.Index(interp_value),
                    {next_geo, meta.time, meta.drift_factor});
  }
}

void
TrailRenderer::BuildCachedSegment(const WindowProjection &projection,
                                  const size_t leg_index,
                                  TrailSettings::Type type,
                                  const ColorScale &color_scale,
                                  const bool use_smoothing,
                                  const unsigned num_segments,
                                  const size_t first_smoothed_point,
                                  size_t &merge_sample_index,
                                  CachedTrailSegment &dest) noexcept
{
  assert(leg_index + 1 < trace.size());

  const TracePoint &prev_tp = trace[leg_index];
  const TracePoint &curr_tp = trace[leg_index + 1];
  const GeoPoint gp0 = prev_tp.GetLocation();
  const GeoPoint gp1 = curr_tp.GetLocation();
  dest.colour_runs.clear();

  const TrailPointData prev_data{
    projection.GeoToScreen(gp0),
    type == TrailSettings::Type::ALTITUDE
      ? prev_tp.GetAltitude() : prev_tp.GetVario(),
    prev_tp.GetTime()};
  const TrailPointData curr_data{
    projection.GeoToScreen(gp1),
    type == TrailSettings::Type::ALTITUDE
      ? curr_tp.GetAltitude() : curr_tp.GetVario(),
    curr_tp.GetTime()};

  const bool use_merge_vario =
    type != TrailSettings::Type::ALTITUDE &&
    !merge_vario_samples.empty();

  if (use_merge_vario)
    BuildVarioBreakpoints(prev_data.time, prev_data.value,
                          curr_data.time, curr_data.value,
                          merge_vario_samples, merge_sample_index,
                          vario_breakpoints);
  else
    vario_breakpoints.clear();

  const GeoPoint g0 = leg_index >= 1
    ? trace[leg_index - 1].GetLocation()
    : gp0;
  const GeoPoint g1 = gp0;
  const GeoPoint g2 = gp1;
  const GeoPoint g3 = leg_index + 2 < trace.size()
    ? trace[leg_index + 2].GetLocation()
    : gp1;

  if (use_smoothing && leg_index + 1 > first_smoothed_point) {
    BuildSmoothColourRuns(g0, g1, g2, g3,
                          prev_tp.GetTime(), curr_tp.GetTime(),
                          prev_tp.GetDriftFactor(), curr_tp.GetDriftFactor(),
                          num_segments, prev_data, curr_data, type, color_scale,
                          use_merge_vario, dest.colour_runs);
  } else {
    const PixelPoint p1s = prev_data.point;
    const PixelPoint p2s = curr_data.point;
    BuildDirectSegmentPoints(p1s, p2s, vario_breakpoints,
                             use_merge_vario, interpolated);
    BuildDirectColourRuns(prev_data, curr_data, gp0, gp1,
                          prev_tp.GetDriftFactor(), curr_tp.GetDriftFactor(),
                          interpolated, type, color_scale, use_merge_vario,
                          dest.colour_runs);
  }

  MergeAdjacentColourRuns(dest.colour_runs);
}

void
TrailRenderer::UpdateSegmentCache(const WindowProjection &projection,
                                  TrailSettings::Type type,
                                  const ColorScale &color_scale,
                                  const bool use_smoothing,
                                  const unsigned num_segments,
                                  const size_t first_smoothed_point,
                                  const size_t from_leg,
                                  const bool rebuild) noexcept
{
  if (rebuild) {
    segment_cache.clear();
    merge_sample_search_index = 0;
  }

  if (trace.size() < 2) {
    cached_trace_size = trace.size();
    return;
  }

  const size_t start_leg = rebuild ? 0 : from_leg;
  if (rebuild)
    segment_cache.reserve(trace.size() - 1);
  else if (from_leg < segment_cache.size()) {
    segment_cache.resize(from_leg);
    merge_sample_search_index =
      from_leg < trace.size()
        ? FindMergeSampleIndexAtOrAfter(trace[from_leg].GetTime(),
                                        merge_vario_samples)
        : merge_vario_samples.size();
  }

  for (size_t leg = start_leg; leg + 1 < trace.size(); ++leg) {
    segment_cache.emplace_back();
    BuildCachedSegment(projection, leg, type, color_scale,
                       use_smoothing, num_segments, first_smoothed_point,
                       merge_sample_search_index, segment_cache.back());
  }

  cached_trace_size = trace.size();
}

void
TrailRenderer::DrawOpenLeg(Canvas &canvas,
                           const TrailSettings &settings,
                           const ColorScale &color_scale,
                           const bool scaled_trail,
                           const bool use_smoothing,
                           const unsigned num_segments,
                           const size_t first_smoothed_point,
                           const NMEAInfo &basic,
                           const PixelPoint aircraft_pos) noexcept
{
  if (valid_points.size() < 1)
    return;

  const TrailPointData &last_data = valid_points.back();

  const bool draw_lines =
    !(last_data.value < 0 && IsVarioDotsOnlyMode(settings.type));

  if (!draw_lines)
    return;

  const bool use_merge_vario =
    settings.type != TrailSettings::Type::ALTITUDE &&
    !merge_vario_samples.empty();

  TrailPointData open_end{aircraft_pos, last_data.value,
                          basic.time.Cast<TracePoint::Time>()};

  size_t merge_sample_index = merge_vario_samples.size();
  if (use_merge_vario)
    BuildVarioBreakpoints(last_data.time, last_data.value,
                          basic.time.Cast<TracePoint::Time>(),
                          last_data.value,
                          merge_vario_samples, merge_sample_index,
                          vario_breakpoints);
  else
    vario_breakpoints.clear();

  if (use_smoothing && valid_points.size() > first_smoothed_point &&
      valid_points.size() >= 2) {
    const auto &prev_data = valid_points[valid_points.size() - 2];
    const PixelPoint p0 = valid_points.size() >= 3
      ? valid_points[valid_points.size() - 3].point
      : prev_data.point;
    DrawSmoothTailSegmentInline(canvas, p0, prev_data.point,
                                last_data.point, aircraft_pos,
                                num_segments, last_data, open_end,
                                settings.type, color_scale, scaled_trail,
                                use_merge_vario);
  } else {
    BuildDirectSegmentPoints(last_data.point, aircraft_pos,
                             vario_breakpoints, use_merge_vario,
                             interpolated);
    DrawVarioColouredPolyline(canvas, interpolated, settings.type,
                              last_data, open_end, color_scale,
                              scaled_trail, use_merge_vario,
                              unsigned(interpolated.size() - 1));
  }
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

  const TrailQuery query = MakeTrailQuery(min_time, projection);
  if (!SyncTrace(trace_computer, query))
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

  const auto minmax = GetMinMax(settings.type, trace);
  const ColorScale color_scale =
    ColorScale::FromMinMax(settings.type, minmax.first, minmax.second);

  const bool zoomed_in =
    projection.GetMapScale() <= TRAIL_ZOOMED_OUT_MAP_SCALE;
  const bool scaled_trail = settings.scaling_enabled && zoomed_in;

  const bool use_smoothing =
    settings.type != TrailSettings::Type::VARIO_1_DOTS &&
    settings.type != TrailSettings::Type::VARIO_2_DOTS &&
    zoomed_in;

  valid_points.clear();
  valid_points.reserve(trace.size());

  for (const auto &i : trace) {
    const GeoPoint gp = TrailGeoPoint(i, enable_traildrift, traildrift, basic);
    const PixelPoint pt = projection.GeoToScreen(gp);
    const double value = (settings.type == TrailSettings::Type::ALTITUDE)
      ? i.GetAltitude() : i.GetVario();
    valid_points.push_back({pt, value, i.GetTime()});
  }

  if (valid_points.empty())
    return;

  const size_t first_smoothed_point =
    use_smoothing ? GetFirstSmoothedPointIndex(valid_points.size())
                  : valid_points.size();
  const size_t smoothed_point_count =
    valid_points.size() - first_smoothed_point;
  const unsigned num_segments =
    use_smoothing
      ? GetSmoothingSegments(projection, smoothed_point_count)
      : 0u;

  const TrailDrawFingerprint new_fingerprint{
    projection.GetScale(),
    query.bounds,
    minmax.first,
    minmax.second,
  };

  const bool modify_changed =
    synced_modify_serial != cache_modify_serial;
  const bool fingerprint_changed = !(fingerprint == new_fingerprint);
  const bool settings_changed =
    cached_settings_type != settings.type ||
    cached_scaled_trail != scaled_trail;

  const size_t leg_count = trace.size() >= 2 ? trace.size() - 1 : 0;

  if (modify_changed || fingerprint_changed || settings_changed)
    UpdateSegmentCache(projection, settings.type, color_scale,
                       use_smoothing, num_segments, first_smoothed_point,
                       0, true);
  else if (leg_count > segment_cache.size()) {
    const size_t rebuild_from =
      use_smoothing && leg_count > MAX_SMOOTHED_TRAIL_POINTS
        ? leg_count - MAX_SMOOTHED_TRAIL_POINTS
        : use_smoothing ? 0 : segment_cache.size();
    UpdateSegmentCache(projection, settings.type, color_scale,
                       use_smoothing, num_segments, first_smoothed_point,
                       rebuild_from, false);
  }
  else if (leg_count < segment_cache.size())
    segment_cache.resize(leg_count);

  cached_trace_size = trace.size();

  cache_append_serial = synced_append_serial;
  cache_modify_serial = synced_modify_serial;
  fingerprint = new_fingerprint;
  cached_settings_type = settings.type;
  cached_scaled_trail = scaled_trail;

  for (size_t i = 1; i < valid_points.size(); ++i) {
    const auto &curr_data = valid_points[i];
    const unsigned color_index = color_scale.Index(curr_data.value);

    const bool draw_dots =
      (curr_data.value < 0 &&
       (IsVarioDotsOnlyMode(settings.type) ||
        settings.type == TrailSettings::Type::VARIO_DOTS_AND_LINES ||
        settings.type == TrailSettings::Type::VARIO_EINK)) ||
      (curr_data.value >= 0 &&
       (settings.type == TrailSettings::Type::VARIO_DOTS_AND_LINES ||
        settings.type == TrailSettings::Type::VARIO_EINK));

    if (!draw_dots)
      continue;

    const auto &prev_data = valid_points[i - 1];
    if (curr_data.value < 0) {
      canvas.SelectNullPen();
      canvas.Select(look.trail_brushes[color_index]);
    } else {
      canvas.Select(look.trail_brushes[color_index]);
      canvas.Select(look.trail_pens[color_index]);
    }
    canvas.DrawCircle({(curr_data.point.x + prev_data.point.x) / 2,
                       (curr_data.point.y + prev_data.point.y) / 2},
                      look.trail_widths[color_index]);
  }

  DrawCachedSegments(canvas, projection, settings.type, scaled_trail,
                     enable_traildrift, traildrift, basic, segment_cache);
  DrawOpenLeg(canvas, settings, color_scale, scaled_trail,
              use_smoothing, num_segments, first_smoothed_point,
              basic, pos);
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
TrailRenderer::DrawVarioColouredPolyline(Canvas &canvas,
                                         const std::vector<PixelPoint> &pts,
                                         TrailSettings::Type type,
                                         const TrailPointData &prev_data,
                                         const TrailPointData &curr_data,
                                         const ColorScale &color_scale,
                                         const bool scaled_trail,
                                         const bool use_merge_vario,
                                         const unsigned num_pieces) noexcept
{
  if (pts.size() < 2 || num_pieces == 0)
    return;

  const double piece_count = static_cast<double>(num_pieces);

  size_t run_start = 0;
  unsigned run_color = 0;
  bool run_active = false;

  auto flush_colour_run = [&](size_t run_end) {
    if (!run_active || run_end <= run_start)
      return;
    DrawColourPolyline(canvas, run_color, scaled_trail,
                       pts, run_start, run_end);
  };

  for (unsigned j = 0; j < num_pieces; ++j) {
    const unsigned seg_color_index =
      color_scale.Index(InterpolatePieceValue(type, prev_data.value,
                                              curr_data.value, j,
                                              piece_count, use_merge_vario,
                                              vario_breakpoints));

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
    flush_colour_run(pts.size() - 1);
}

void
TrailRenderer::DrawSmoothTailSegmentInline(
    Canvas &canvas,
    const PixelPoint &p0, const PixelPoint &p1,
    const PixelPoint &p2, const PixelPoint &p3,
    unsigned num_segments,
    const TrailPointData &prev_data,
    const TrailPointData &curr_data,
    TrailSettings::Type type,
    const ColorScale &color_scale,
    bool scaled_trail,
    bool use_merge_vario) noexcept
{
  const double piece_count = static_cast<double>(num_segments);

  interpolated.clear();
  interpolated.push_back(p1);

  for (unsigned j = 0; j < num_segments; ++j) {
    const unsigned next_piece = j + 1;
    const double t = static_cast<double>(next_piece) / piece_count;
    interpolated.push_back(
      next_piece == num_segments
        ? p2
        : CatmullRomInterpolate(p0, p1, p2, p3, t));
  }

  DrawVarioColouredPolyline(canvas, interpolated, type,
                            prev_data, curr_data, color_scale,
                            scaled_trail, use_merge_vario, num_segments);
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
