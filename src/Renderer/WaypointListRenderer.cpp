// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "util/Macros.hpp"

typedef StaticString<256u> Buffer;

static void
FormatWaypointDetails(Buffer &buffer, const Waypoint &waypoint)
{
  if (waypoint.has_elevation)
    buffer.Format("%s: %s", _("Elevation"),
                  FormatUserAltitude(waypoint.elevation).c_str());
  else
    buffer.Format("%s: %s", _("Elevation"), "?");

  if (waypoint.radio_frequency.IsDefined()) {
    char radio[16];
    waypoint.radio_frequency.Format(radio, 16);
    buffer.AppendFormat(" - %s MHz", radio);
  }

  if (!waypoint.comment.empty()) {
    buffer.AppendFormat(" - %s", waypoint.comment.c_str());
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
    const int distance_x = row_renderer.DrawRightFirstRow(canvas, rc,
                                                          FormatUserDistanceSmart(vector->distance, true));

    // Draw leg bearing
    const int bearing_x = row_renderer.DrawRightSecondRow(canvas, rc,
                                                          FormatBearing(vector->bearing));

    rc.right = std::min(distance_x, bearing_x);
  }

  // Draw details line
  FormatWaypointDetails(buffer, waypoint);
  row_renderer.DrawSecondRow(canvas, rc, buffer);

  // Draw waypoint name
  if (!waypoint.shortname.empty()) {
    const auto waypoint_title = waypoint.name +
      " (" + waypoint.shortname + ")";
    row_renderer.DrawFirstRow(canvas, rc, waypoint_title.c_str());
  }
  else {
    row_renderer.DrawFirstRow(canvas, rc, waypoint.name.c_str());
  }
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

  const auto reachable = arrival_altitude > 0
    ? WaypointReachability::TERRAIN
    : WaypointReachability::UNREACHABLE;

  WaypointIconRenderer wir(settings, look, canvas);
  wir.Draw(waypoint, pt, reachable);

  rc.left += line_height + padding;

  // Draw distance and arrival altitude
  StaticString<256> buffer;
  char alt[20], radio[20];
  
  FormatRelativeUserAltitude(arrival_altitude, alt, true);
  buffer.Format("%s: %s - %s: %s", _("Distance"),
                FormatUserDistanceSmart(distance).c_str(),
                _("Arrival Alt"), alt);

  if (waypoint.radio_frequency.IsDefined()) {
    waypoint.radio_frequency.Format(radio, ARRAY_SIZE(radio));
    buffer.AppendFormat(" - %s MHz", radio);
  }

  row_renderer.DrawSecondRow(canvas, rc, buffer);

  // Draw waypoint name
  if (!waypoint.shortname.empty()) {
    const auto waypoint_title = waypoint.name +
      _(" (") + waypoint.shortname + ")";
    row_renderer.DrawFirstRow(canvas, rc, waypoint_title.c_str());
  }
  else {
    row_renderer.DrawFirstRow(canvas, rc, waypoint.name.c_str());
  }
}
