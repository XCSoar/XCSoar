/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "Look/WaypointLook.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "Util/StaticString.hpp"
#include "Util/Macros.hpp"

#include <cstdio>

namespace WaypointListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            const GeoVector *vector,
            const DialogLook &dialog_look, const WaypointLook &look,
            const WaypointRendererSettings &settings);
}

typedef StaticString<256u> Buffer;

static void
FormatWaypointDetails(Buffer &buffer, const Waypoint &waypoint)
{
  TCHAR alt[16];
  FormatUserAltitude(waypoint.elevation, alt, 16);
  buffer.Format(_T("%s: %s"), _("Elevation"), alt);

  if (waypoint.radio_frequency.IsDefined()) {
    TCHAR radio[16];
    waypoint.radio_frequency.Format(radio, 16);
    buffer.AppendFormat(_T(" - %s MHz"), radio);
  }

  if (!waypoint.comment.empty()) {
    buffer.AppendFormat(_T(" - %s"), waypoint.comment.c_str());
  }
}

UPixelScalar
WaypointListRenderer::GetHeight(const DialogLook &look)
{
  return look.list.font->GetHeight() + Layout::Scale(6)
    + look.small_font->GetHeight();
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint,
                           const DialogLook &dialog_look,
                           const WaypointLook &look,
                           const WaypointRendererSettings &renderer_settings)
{
  Draw(canvas, rc, waypoint, NULL, dialog_look, look, renderer_settings);
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, const GeoVector &vector,
                           const DialogLook &dialog_look,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  Draw(canvas, rc, waypoint, &vector, dialog_look, look, settings);
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, fixed distance,
                           fixed arrival_altitude,
                           const DialogLook &dialog_look,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.Select(small_font);

  // Draw distance and arrival altitude
  StaticString<256> buffer;
  TCHAR dist[20], alt[20], radio[20];
  FormatUserDistanceSmart(distance, dist, true);
  FormatRelativeUserAltitude(arrival_altitude, alt, true);
  buffer.Format(_T("%s: %s - %s: %s"), _("Distance"), dist,
                _("Arrival Alt"), alt);

  if (waypoint.radio_frequency.IsDefined()) {
    waypoint.radio_frequency.Format(radio, ARRAY_SIZE(radio));
    buffer.AppendFormat(_T(" - %s MHz"), radio);
  }

  UPixelScalar left = rc.left + line_height + Layout::FastScale(2);
  canvas.DrawClippedText(left, top2, rc, buffer);

  // Draw waypoint name
  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + Layout::FastScale(2), rc,
                         waypoint.name.c_str());

  // Draw icon
  const RasterPoint pt(rc.left + line_height / 2,
                       rc.top + line_height / 2);

  WaypointIconRenderer::Reachability reachable =
      positive(arrival_altitude) ?
      WaypointIconRenderer::ReachableTerrain : WaypointIconRenderer::Unreachable;

  WaypointIconRenderer wir(settings, look, canvas);
  wir.Draw(waypoint, pt, reachable);
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, const GeoVector *vector,
                           const DialogLook &dialog_look,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  Buffer buffer;

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.Select(small_font);

  // Draw leg distance
  UPixelScalar leg_info_width = 0;
  if (vector) {
    FormatUserDistanceSmart(vector->distance, buffer.buffer(), true);
    UPixelScalar width = leg_info_width = canvas.CalcTextWidth(buffer.c_str());
    canvas.DrawText(rc.right - Layout::FastScale(2) - width,
                    rc.top + Layout::FastScale(2) +
                    (name_font.GetHeight() - small_font.GetHeight()) / 2,
                    buffer.c_str());

    // Draw leg bearing
    FormatBearing(buffer.buffer(), buffer.MAX_SIZE, vector->bearing);
    width = canvas.CalcTextWidth(buffer.c_str());
    canvas.DrawText(rc.right - Layout::FastScale(2) - width, top2,
                    buffer.c_str());

    if (width > leg_info_width)
      leg_info_width = width;

    leg_info_width += Layout::FastScale(2);
  }

  // Draw details line
  FormatWaypointDetails(buffer, waypoint);

  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  canvas.DrawClippedText(left, top2, rc.right - leg_info_width - left,
                         buffer.c_str());

  // Draw waypoint name
  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + Layout::FastScale(2),
                         rc.right - leg_info_width - left,
                         waypoint.name.c_str());

  // Draw icon
  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };
  WaypointIconRenderer wir(settings, look, canvas);
  wir.Draw(waypoint, pt);
}
