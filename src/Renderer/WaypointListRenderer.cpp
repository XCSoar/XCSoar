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

#include "WaypointListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Language/Language.hpp"
#include "Util/StaticString.hxx"
#include "Util/Macros.hpp"

typedef StaticString<256u> Buffer;

static void
FormatWaypointDetails(Buffer &buffer, const Waypoint &waypoint)
{
  buffer.Format(_T("%s: %s"), _("Elevation"),
                FormatUserAltitude(waypoint.elevation).c_str());

  if (waypoint.radio_frequency.IsDefined()) {
    TCHAR radio[16];
    waypoint.radio_frequency.Format(radio, 16);
    buffer.AppendFormat(_T(" - %s MHz"), radio);
  }

  if (!waypoint.comment.empty()) {
    buffer.AppendFormat(_T(" - %s"), waypoint.comment.c_str());
  }
}

static void
Draw(Canvas &canvas, PixelRect rc,
     const Waypoint &waypoint, const GeoVector *vector,
     const TwoTextRowsRenderer &row_renderer,
     const WaypointLook &look,
     const WaypointRendererSettings &settings)
{
  const unsigned padding = Layout::GetTextPadding();
  const unsigned line_height = rc.GetHeight();

  // Draw icon
  const PixelPoint pt(rc.left + line_height / 2, rc.top + line_height / 2);
  WaypointIconRenderer wir(settings, look, canvas);
  wir.Draw(waypoint, pt);

  rc.left += line_height + padding;

  Buffer buffer;

  if (vector) {
    // Draw leg distance
    FormatUserDistanceSmart(vector->distance, buffer.buffer(), true);
    const int distance_x = row_renderer.DrawRightFirstRow(canvas, rc, buffer);

    // Draw leg bearing
    FormatBearing(buffer.buffer(), buffer.capacity(), vector->bearing);
    const int bearing_x = row_renderer.DrawRightSecondRow(canvas, rc, buffer);

    rc.right = std::min(distance_x, bearing_x);
  }

  // Draw details line
  FormatWaypointDetails(buffer, waypoint);
  row_renderer.DrawSecondRow(canvas, rc, buffer);

  // Draw waypoint name
  row_renderer.DrawFirstRow(canvas, rc, waypoint.name.c_str());
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint,
                           const TwoTextRowsRenderer &row_renderer,
                           const WaypointLook &look,
                           const WaypointRendererSettings &renderer_settings)
{
  ::Draw(canvas, rc, waypoint, nullptr, row_renderer, look, renderer_settings);
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, const GeoVector &vector,
                           const TwoTextRowsRenderer &row_renderer,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  ::Draw(canvas, rc, waypoint, &vector, row_renderer, look, settings);
}

void
WaypointListRenderer::Draw(Canvas &canvas, PixelRect rc,
                           const Waypoint &waypoint, double distance,
                           double arrival_altitude,
                           const TwoTextRowsRenderer &row_renderer,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  const unsigned padding = Layout::GetTextPadding();
  const unsigned line_height = rc.GetHeight();

  // Draw icon
  const PixelPoint pt(rc.left + line_height / 2,
                      rc.top + line_height / 2);

  WaypointIconRenderer::Reachability reachable = arrival_altitude > 0
    ? WaypointIconRenderer::ReachableTerrain
    : WaypointIconRenderer::Unreachable;

  WaypointIconRenderer wir(settings, look, canvas);
  wir.Draw(waypoint, pt, reachable);

  rc.left += line_height + padding;

  // Draw distance and arrival altitude
  StaticString<256> buffer;
  TCHAR alt[20], radio[20];
  
  FormatRelativeUserAltitude(arrival_altitude, alt, true);
  buffer.Format(_T("%s: %s - %s: %s"), _("Distance"),
                FormatUserDistanceSmart(distance).c_str(),
                _("Arrival Alt"), alt);

  if (waypoint.radio_frequency.IsDefined()) {
    waypoint.radio_frequency.Format(radio, ARRAY_SIZE(radio));
    buffer.AppendFormat(_T(" - %s MHz"), radio);
  }

  row_renderer.DrawSecondRow(canvas, rc, buffer);

  // Draw waypoint name
  row_renderer.DrawFirstRow(canvas, rc, waypoint.name.c_str());
}
