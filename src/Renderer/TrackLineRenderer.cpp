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

#include "TrackLineRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Screen/Canvas.hpp"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "MapSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/Math.hpp"

#define ARC_STEPS 10
static constexpr Angle ARC_SWEEP = Angle::Degrees(135.0);
static constexpr Angle MIN_RATE = Angle::Degrees(1.0); // degrees/s

void
TrackLineRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const Angle track_angle, const PixelPoint pos)
{
  const auto sc = (track_angle - screen_angle).SinCos();
  const auto x = sc.first, y = sc.second;

  PixelPoint end;
  end.x = pos.x + iround(x * 400);
  end.y = pos.y - iround(y * 400);

  canvas.Select(look.track_line_pen);
  canvas.DrawLine(pos, end);
}

void
TrackLineRenderer::Draw(Canvas &canvas,
                        const WindowProjection &projection,
                        const PixelPoint pos, const NMEAInfo &basic,
                        const DerivedInfo &calculated,
                        const MapSettings &settings,
                        bool wind_relative)
{
  if (!basic.track_available || !basic.attitude.IsHeadingUseable())
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
      (basic.track - basic.attitude.heading).AsDelta().AbsoluteDegrees() < 5)
    return;

  TrackLineRenderer::Draw(canvas, projection.GetScreenAngle(), basic.track, pos);
}

void
TrackLineRenderer::DrawProjected(Canvas &canvas,
                                 const WindowProjection &projection,
                                 const NMEAInfo &basic,
                                 const DerivedInfo &calculated,
                                 const MapSettings &settings,
                                 bool wind_relative)
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
  int i = 1;

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
