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

#include "MapItemListRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "MapWindow/MapItem.hpp"
#include "Look/AirspaceLook.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Look/WaypointLook.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Units/UnitsFormatter.hpp"
#include "SettingsMap.hpp"

#include <cstdio>

namespace MapItemListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const AirspaceMapItem &item,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const WaypointMapItem &item,
            const WaypointLook &look,
            const WaypointRendererSettings &renderer_settings);
}

void
MapItemListRenderer::Draw(
    Canvas &canvas, const PixelRect rc, const AirspaceMapItem &item,
    const AirspaceLook &look,
    const AirspaceRendererSettings &renderer_settings)
{
  const unsigned line_height = rc.bottom - rc.top;

  const AbstractAirspace &airspace = *item.airspace;

  RasterPoint pt = { rc.left + line_height / 2,
                     rc.top + line_height / 2};
  unsigned radius = std::min(line_height / 2  - Layout::FastScale(4),
                             (unsigned)Layout::FastScale(10));
  AirspacePreviewRenderer::Draw(canvas, airspace, pt, radius,
                                renderer_settings, look);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  unsigned left = rc.left + line_height + Layout::FastScale(2);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      airspace.GetName());

  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, airspace.GetTypeText(false));

  unsigned altitude_width = canvas.text_width(airspace.GetTopText(true).c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.get_height() -
                      small_font.get_height() + Layout::FastScale(2), rc,
                      airspace.GetTopText(true).c_str());

  altitude_width = canvas.text_width(airspace.GetBaseText(true).c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, airspace.GetBaseText(true).c_str());
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const WaypointMapItem &item, const WaypointLook &look,
                          const WaypointRendererSettings &renderer_settings)
{
  const unsigned line_height = rc.bottom - rc.top;

  const Waypoint &waypoint = item.waypoint;

  RasterPoint pt = { rc.left + line_height / 2,
                     rc.top + line_height / 2};
  WaypointIconRenderer wir(renderer_settings, look, canvas);
  wir.Draw(waypoint, pt);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  unsigned left = rc.left + line_height + Layout::FastScale(2);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      waypoint.name.c_str());

  TCHAR buffer[256];
  {
    TCHAR alt[16];
    Units::FormatUserAltitude(waypoint.altitude, alt, 16);
    _stprintf(buffer, _T("%s: %s"), _T("Altitude"), alt);
  }

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

  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, buffer);
}

void
MapItemListRenderer::Draw(
    Canvas &canvas, const PixelRect rc, const MapItem &item,
    const AirspaceLook &airspace_look,
    const WaypointLook &waypoint_look,
    const SETTINGS_MAP &settings)
{
  switch (item.type) {
  case MapItem::AIRSPACE:
    Draw(canvas, rc, (const AirspaceMapItem &)item, airspace_look,
         settings.airspace);
    break;
  case MapItem::WAYPOINT:
    Draw(canvas, rc, (const WaypointMapItem &)item, waypoint_look,
         settings.waypoint);
    break;
  }
}
