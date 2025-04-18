// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackLineRenderer.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "MapSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/Math.hpp"
#include "Screen/Layout.hpp"

static constexpr unsigned ARC_STEPS = 10;
static constexpr Angle ARC_SWEEP = Angle::Degrees(135.0);
static constexpr Angle MIN_RATE = Angle::Degrees(1.0); // degrees/s

void
TrackLineRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const Angle track_angle, const PixelPoint pos) noexcept
{
  const auto sc = (track_angle - screen_angle).SinCos();
  const auto x = sc.first, y = sc.second;

  PixelPoint end;
  const int scaled_length = Layout::Scale(400);
  end.x = pos.x + iround(x * scaled_length);
  end.y = pos.y - iround(y * scaled_length);

  canvas.Select(look.track_line_pen);
  canvas.DrawLine(pos, end);
}

void
TrackLineRenderer::Draw(Canvas &canvas,
                        const WindowProjection &projection,
                        const PixelPoint pos, const NMEAInfo &basic,
                        const DerivedInfo &calculated,
                        const MapSettings &settings,
                        bool wind_relative) noexcept
{
  if (!basic.track_available || !basic.attitude.heading_available)
    return;

  if (basic.airspeed_available.IsValid() &&
      (calculated.turn_rate_heading_smoothed.Absolute()>= MIN_RATE)) {
    TrackLineRenderer::DrawProjected(canvas, projection, basic, calculated, settings,
      wind_relative);
  }

  if (settings.display_ground_track == DisplayGroundTrack::OFF ||
      calculated.circling)
    return;

  if (settings.display_ground_track == DisplayGroundTrack::AUTO &&
      (basic.track - basic.attitude.heading).AsDelta().Absolute() < Angle::Degrees(5))
    return;

  TrackLineRenderer::Draw(canvas, projection.GetScreenAngle(), basic.track, pos);
}

inline void
TrackLineRenderer::DrawProjected(Canvas &canvas,
                                 const WindowProjection &projection,
                                 const NMEAInfo &basic,
                                 const DerivedInfo &calculated,
                                 [[maybe_unused]] const MapSettings &settings,
                                 bool wind_relative) noexcept
{
  // projection.GetMapScale() <= 6000;

  GeoPoint traildrift;

  if (calculated.wind_available && !wind_relative) {
    GeoPoint tp1 = FindLatitudeLongitude(basic.location,
                                         calculated.wind.bearing,
                                         calculated.wind.norm);
    traildrift = basic.location - tp1;
  } else {
    traildrift = GeoPoint(Angle::Zero(), Angle::Zero());
  }

  auto dt = ARC_SWEEP/ARC_STEPS/
    std::max(MIN_RATE,calculated.turn_rate_heading_smoothed.Absolute());

  Angle heading = basic.attitude.heading;
  GeoPoint loc = basic.location;

  BulkPixelPoint pts[ARC_STEPS+1];
  pts[0] = projection.GeoToScreen(loc);
  unsigned i = 1;

  while (i <= ARC_STEPS) {
    GeoVector v(basic.true_airspeed*dt, heading);
    loc = v.EndPoint(loc.Parametric(traildrift, dt));
    pts[i] = projection.GeoToScreen(loc);
    heading += calculated.turn_rate_heading_smoothed*dt;
    i++;
  }
  canvas.Select(look.track_line_pen);
  canvas.DrawPolyline(pts, i);
}
