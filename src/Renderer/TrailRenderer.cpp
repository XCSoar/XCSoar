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

#include <algorithm>

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

  PixelPoint last_point(0, 0);
  bool last_valid = false;
  for (const auto &i : trace) {
    const GeoPoint gp = enable_traildrift
      ? i.GetLocation().Parametric(traildrift, i.CalculateDrift(basic.time))
      : i.GetLocation();
    if (!bounds.IsInside(gp)) {
      /* the point is outside of the MapWindow; don't paint it */
      last_valid = false;
      continue;
    }

    auto pt = projection.GeoToScreen(gp);

    if (last_valid) {
      if (settings.type == TrailSettings::Type::ALTITUDE) {
        unsigned index = GetAltitudeColorIndex(i.GetAltitude(),
                                               value_min, value_max);
        canvas.Select(look.trail_pens[index]);
        canvas.DrawLinePiece(last_point, pt);
      } else {
        unsigned color_index = GetSnailColorIndex(i.GetVario(),
                                                  value_min, value_max);
        if (i.GetVario() < 0 &&
            (settings.type == TrailSettings::Type::VARIO_1_DOTS ||
             settings.type == TrailSettings::Type::VARIO_2_DOTS ||
             settings.type == TrailSettings::Type::VARIO_DOTS_AND_LINES ||
             settings.type == TrailSettings::Type::VARIO_EINK)) {
          canvas.SelectNullPen();
          canvas.Select(look.trail_brushes[color_index]);
          canvas.DrawCircle({(pt.x + last_point.x) / 2, (pt.y + last_point.y) / 2},
                            look.trail_widths[color_index]);
        } else {
          // positive vario case
          if (settings.type == TrailSettings::Type::VARIO_DOTS_AND_LINES ||
              settings.type == TrailSettings::Type::VARIO_EINK) {
            canvas.Select(look.trail_brushes[color_index]);
            canvas.Select(look.trail_pens[color_index]); //fixed-width pen
            canvas.DrawCircle({(pt.x + last_point.x) / 2, (pt.y + last_point.y) / 2},
                            look.trail_widths[color_index]);
          } else if (scaled_trail)
            // width scaled to vario
            canvas.Select(look.scaled_trail_pens[color_index]);
          else
            // fixed-width pen
            canvas.Select(look.trail_pens[color_index]);
          canvas.DrawLinePiece(last_point, pt);
        }
      }
    }
    last_point = pt;
    last_valid = true;
  }

  if (last_valid)
    canvas.DrawLine(last_point, pos);
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
