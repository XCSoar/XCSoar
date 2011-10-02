/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/Fonts.hpp"
#include "Look/WaypointLook.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Language/Language.hpp"
#include "SettingsMap.hpp"

#include <cstdio>

namespace WaypointListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const Waypoint &waypoint,
            const GeoVector *vector, const WaypointLook &look,
            const WaypointRendererSettings &settings);
}

static void
FormatWaypointDetails(TCHAR *buffer, const Waypoint &waypoint)
{
  TCHAR alt[16];
  Units::FormatUserAltitude(waypoint.altitude, alt, 16);
  _stprintf(buffer, _T("%s: %s"), _("Altitude"), alt);

  if (waypoint.radio_frequency.IsDefined()) {
    TCHAR radio[16];
    waypoint.radio_frequency.Format(radio, 16);
    _tcscat(buffer, _T(" - "));
    _tcscat(buffer, radio);
    _tcscat(buffer, _T(" MHz"));
  }

  if (!waypoint.comment.empty()) {
    _tcscat(buffer, _T(" - "));
    _tcscat(buffer, waypoint.comment.c_str());
  }
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, const WaypointLook &look,
                           const WaypointRendererSettings &renderer_settings)
{
  Draw(canvas, rc, waypoint, NULL, look, renderer_settings);
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, const GeoVector &vector,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  Draw(canvas, rc, waypoint, &vector, look, settings);
}

void
WaypointListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const Waypoint &waypoint, const GeoVector *vector,
                           const WaypointLook &look,
                           const WaypointRendererSettings &settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  TCHAR buffer[256];

  // Draw icon
  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };
  WaypointIconRenderer wir(settings, look, canvas);
  wir.Draw(waypoint, pt);

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.get_height() + Layout::FastScale(4);

  // Use small font for details
  canvas.select(small_font);

  // Draw leg distance
  unsigned leg_info_width = 0;
  if (vector) {
    Units::FormatUserDistance(vector->distance, buffer, 256, true);
    unsigned width = leg_info_width = canvas.text_width(buffer);
    canvas.text(rc.right - Layout::FastScale(2) - width,
                rc.top + Layout::FastScale(2) +
                (name_font.get_height() - small_font.get_height()) / 2, buffer);

    // Draw leg bearing
    _stprintf(buffer, _T(" %.0f" DEG " T"),
              (double)vector->bearing.Degrees());
    width = canvas.text_width(buffer);
    canvas.text(rc.right - Layout::FastScale(2) - width, top2, buffer);

    if (width > leg_info_width)
      leg_info_width = width;

    leg_info_width += Layout::FastScale(2);
  }

  // Draw details line
  FormatWaypointDetails(buffer, waypoint);

  unsigned left = rc.left + line_height + Layout::FastScale(2);
  canvas.text_clipped(left, top2, rc.right - leg_info_width - left, buffer);

  // Draw waypoint name
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2),
                      rc.right - leg_info_width - left, waypoint.name.c_str());
}
