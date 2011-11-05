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
#include "Look/AircraftLook.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Look/AirspaceLook.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Renderer/OZPreviewRenderer.hpp"
#include "Look/TaskLook.hpp"
#include "Look/MarkerLook.hpp"
#include "Language/Language.hpp"
#include "Util/StringUtil.hpp"
#include "SettingsMap.hpp"
#include "LocalTime.hpp"
#include "Math/Screen.hpp"
#include "Look/TrafficLook.hpp"
#include "Renderer/TrafficRenderer.hpp"

#include <cstdio>

namespace MapItemListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const SelfMapItem &item,
            const AircraftLook &look, const SETTINGS_MAP &settings);

  void Draw(Canvas &canvas, const PixelRect rc, const AirspaceMapItem &item,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const WaypointMapItem &item,
            const WaypointLook &look,
            const WaypointRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const MarkerMapItem &item,
            const MarkerLook &look);

  void Draw(Canvas &canvas, const PixelRect rc, const TaskOZMapItem &item,
            const TaskLook &look, const AirspaceLook &airspace_look,
            const AirspaceRendererSettings &airspace_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const TrafficMapItem &item,
            const TrafficLook &traffic_look);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const SelfMapItem &item, const AircraftLook &look,
                          const SETTINGS_MAP &settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };
  AircraftRenderer::Draw(canvas, settings, look, item.bearing, pt);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;
  canvas.set_text_color(COLOR_BLACK);

  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      _("Your Position"));

  TCHAR buffer[128];
  Units::FormatGeoPoint(item.location, buffer, 128);

  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, buffer);
}

void
MapItemListRenderer::Draw(
    Canvas &canvas, const PixelRect rc, const AirspaceMapItem &item,
    const AirspaceLook &look,
    const AirspaceRendererSettings &renderer_settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const AbstractAirspace &airspace = *item.airspace;

  RasterPoint pt = { PixelScalar(rc.left + line_height / 2),
                     PixelScalar(rc.top + line_height / 2) };
  PixelScalar radius = std::min(PixelScalar(line_height / 2
                                            - Layout::FastScale(4)),
                                Layout::FastScale(10));
  AirspacePreviewRenderer::Draw(canvas, airspace, pt, radius,
                                renderer_settings, look);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;
  canvas.set_text_color(COLOR_BLACK);

  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      airspace.GetName());

  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, airspace.GetTypeText(false));

  PixelScalar altitude_width =
    canvas.text_width(airspace.GetTopText(true).c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.GetHeight() -
                      small_font.GetHeight() + Layout::FastScale(2), rc,
                      airspace.GetTopText(true).c_str());

  altitude_width = canvas.text_width(airspace.GetBaseText(true).c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, airspace.GetBaseText(true).c_str());
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const WaypointMapItem &item, const WaypointLook &look,
                          const WaypointRendererSettings &renderer_settings)
{
  WaypointListRenderer::Draw(canvas, rc, item.waypoint, look, renderer_settings);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const MarkerMapItem &item, const MarkerLook &look)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const Markers::Marker &marker = item.marker;

  RasterPoint pt = { PixelScalar(rc.left + line_height / 2),
                     PixelScalar(rc.top + line_height / 2) };

  look.icon.Draw(canvas, pt);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;
  canvas.set_text_color(COLOR_BLACK);

  PixelScalar left = rc.left + line_height + Layout::FastScale(2);

  TCHAR buffer[256];
  _stprintf(buffer, _T("%s #%d"), _("Marker"), item.id + 1);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc, buffer);

  TCHAR time_buffer[32];
  Units::TimeToTextHHMMSigned(time_buffer, TimeLocal(marker.time.GetSecondOfDay()));
  _stprintf(buffer, _("dropped at %s"), time_buffer);
  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, buffer);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const TaskOZMapItem &item,
                          const TaskLook &look, const AirspaceLook &airspace_look,
                          const AirspaceRendererSettings &airspace_settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const ObservationZonePoint &oz = *item.oz;
  const Waypoint &waypoint = item.waypoint;

  RasterPoint pt = { PixelScalar(rc.left + line_height / 2),
                     PixelScalar(rc.top + line_height / 2) };
  PixelScalar radius = std::min(PixelScalar(line_height / 2
                                            - Layout::FastScale(4)),
                                Layout::FastScale(10));
  OZPreviewRenderer::Draw(canvas, oz, pt, radius, look,
                          airspace_settings, airspace_look);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;
  canvas.set_text_color(COLOR_BLACK);

  TCHAR buffer[256];

  // Y-Coordinate of the second row
  UPixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.select(small_font);

  // Draw details line
  UPixelScalar left = rc.left + line_height + Layout::FastScale(2);
  OrderedTaskPointRadiusLabel(*item.oz, buffer);
  if (!string_is_empty(buffer))
    canvas.text_clipped(left, top2, rc.right - left, buffer);

  // Draw waypoint name
  canvas.select(name_font);
  OrderedTaskPointLabel(item.tp_type, waypoint.name.c_str(),
                        item.index, buffer);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2),
                      rc.right - left, buffer);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const TrafficMapItem &item,
                          const TrafficLook &traffic_look)
{
  const PixelScalar line_height = rc.bottom - rc.top;
  const FlarmTraffic traffic = item.traffic;

  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };

  // Render the representation of the traffic icon
  TrafficRenderer::Draw(canvas, traffic_look, traffic, traffic.track, pt);

  // Now render the text information
  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;
  canvas.set_text_color(COLOR_BLACK);

  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      _("FLARM Traffic"));

  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, FlarmTraffic::GetTypeString(item.traffic.type));
}

void
MapItemListRenderer::Draw(
    Canvas &canvas, const PixelRect rc, const MapItem &item,
    const AircraftLook &aircraft_look,
    const AirspaceLook &airspace_look,
    const WaypointLook &waypoint_look,
    const TaskLook &task_look,
    const MarkerLook &marker_look,
    const TrafficLook &traffic_look,
    const SETTINGS_MAP &settings)
{
  switch (item.type) {
  case MapItem::SELF:
    Draw(canvas, rc, (const SelfMapItem &)item, aircraft_look, settings);
    break;
  case MapItem::AIRSPACE:
    Draw(canvas, rc, (const AirspaceMapItem &)item, airspace_look,
         settings.airspace);
    break;
  case MapItem::WAYPOINT:
    Draw(canvas, rc, (const WaypointMapItem &)item, waypoint_look,
         settings.waypoint);
    break;
  case MapItem::TASK_OZ:
    Draw(canvas, rc, (const TaskOZMapItem &)item, task_look, airspace_look,
         settings.airspace);
    break;
  case MapItem::MARKER:
    Draw(canvas, rc, (const MarkerMapItem &)item, marker_look);
    break;
  case MapItem::TRAFFIC:
    Draw(canvas, rc, (const TrafficMapItem &)item, traffic_look);
    break;
  }
}
