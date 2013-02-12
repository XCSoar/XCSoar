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

#include "MapItemListRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "Look/DialogLook.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/AirspaceListRenderer.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Dialogs/Task/dlgTaskHelpers.hpp"
#include "Renderer/OZPreviewRenderer.hpp"
#include "Language/Language.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"
#include "Util/StaticString.hpp"
#include "Terrain/RasterBuffer.hpp"
#include "MapSettings.hpp"
#include "LocalTime.hpp"
#include "Math/Screen.hpp"
#include "Look/TrafficLook.hpp"
#include "Look/FinalGlideBarLook.hpp"
#include "Renderer/TrafficRenderer.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/FlarmNetRecord.hpp"
#include "Weather/Features.hpp"
#include "FLARM/List.hpp"

#ifdef HAVE_NOAA
#include "Renderer/NOAAListRenderer.hpp"
#endif

#include <cstdio>

namespace MapItemListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const LocationMapItem &item,
            const DialogLook &dialog_look);

  void Draw(Canvas &canvas, const PixelRect rc,
            const ArrivalAltitudeMapItem &item,
            const DialogLook &dialog_look, const FinalGlideBarLook &look);

  void Draw(Canvas &canvas, const PixelRect rc, const SelfMapItem &item,
            const DialogLook &dialog_look,
            const AircraftLook &look, const MapSettings &settings);

  void Draw(Canvas &canvas, const PixelRect rc, const AirspaceMapItem &item,
            const DialogLook &dialog_look, const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const WaypointMapItem &item,
            const DialogLook &dialog_look, const WaypointLook &look,
            const WaypointRendererSettings &renderer_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const MarkerMapItem &item,
            const DialogLook &dialog_look, const MarkerLook &look);

#ifdef HAVE_NOAA
  void Draw(Canvas &canvas, const PixelRect rc, const WeatherStationMapItem &item,
            const DialogLook &dialog_look, const NOAALook &look);
#endif

  void Draw(Canvas &canvas, const PixelRect rc, const TaskOZMapItem &item,
            const DialogLook &dialog_look,
            const TaskLook &look, const AirspaceLook &airspace_look,
            const AirspaceRendererSettings &airspace_settings);

  void Draw(Canvas &canvas, const PixelRect rc, const TrafficMapItem &item,
            const DialogLook &dialog_look, const TrafficLook &traffic_look,
            const TrafficList *traffic_list);

  void Draw(Canvas &canvas, const PixelRect rc, const ThermalMapItem &item,
            const DialogLook &dialog_look, const MapLook &look);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const LocationMapItem &item,
                          const DialogLook &dialog_look)
{
  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  const unsigned text_padding = Layout::GetTextPadding();
  int left = rc.left + text_padding;

  TCHAR info_buffer[256], distance_buffer[32], direction_buffer[32];
  if (item.vector.IsValid()) {
    FormatUserDistanceSmart(item.vector.distance, distance_buffer, 32);
    FormatBearing(direction_buffer, ARRAY_SIZE(direction_buffer),
                  item.vector.bearing);
    _stprintf(info_buffer, _T("%s: %s, %s: %s"),
              _("Distance"), distance_buffer,
              _("Direction"), direction_buffer);
  } else {
    _stprintf(info_buffer, _T("%s: %s, %s: %s"),
              _("Distance"), _T("???"), _("Direction"), _T("???"));
  }

  canvas.Select(name_font);

  canvas.DrawClippedText(left, rc.top + text_padding, rc, info_buffer);


  TCHAR elevation_buffer[32];
  if (!RasterBuffer::IsSpecial(item.elevation)) {
    FormatUserAltitude(fixed(item.elevation), elevation_buffer, 32);
    _stprintf(info_buffer, _T("%s: %s"), _("Elevation"), elevation_buffer);
  } else {
    _stprintf(info_buffer, _T("%s: %s"), _("Elevation"), _T("???"));
  }

  canvas.Select(small_font);
  canvas.DrawClippedText(left,
                         rc.top + name_font.GetHeight() + 2 * text_padding,
                         rc, info_buffer);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const ArrivalAltitudeMapItem &item,
                          const DialogLook &dialog_look,
                          const FinalGlideBarLook &look)
{
  const UPixelScalar line_height = rc.bottom - rc.top;

  bool elevation_available =
      !RasterBuffer::IsSpecial((short)item.elevation);

  bool reach_relevant = item.reach.IsReachRelevant();

  RoughAltitude arrival_altitude =
    item.reach.terrain_valid == ReachResult::Validity::VALID
    ? item.reach.terrain
    : item.reach.direct;
  if (elevation_available)
    arrival_altitude -= item.elevation;

  bool reachable =
    item.reach.terrain_valid != ReachResult::Validity::UNREACHABLE &&
    arrival_altitude.IsPositive();

  // Draw final glide arrow icon

  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };

  RasterPoint arrow[] = {
      { -7, -3 }, { 0, 4 }, { 7, -3 }
  };

  Angle arrow_angle = reachable ? Angle::HalfCircle() : Angle::Zero();
  PolygonRotateShift(arrow, ARRAY_SIZE(arrow), pt.x, pt.y, arrow_angle, 100);

  if (reachable) {
    canvas.Select(look.brush_above);
    canvas.Select(look.pen_above);
  } else {
    canvas.Select(look.brush_below);
    canvas.Select(look.pen_below);
  }
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));


  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  const unsigned text_padding = Layout::GetTextPadding();
  int left = rc.left + line_height + text_padding;


  // Format title row

  TCHAR altitude_buffer[32];
  StaticString<256> buffer;
  buffer.clear();

  if (elevation_available) {
    RoughAltitude relative_arrival_altitude =
      item.reach.direct - item.elevation;

    FormatRelativeUserAltitude(fixed((short)relative_arrival_altitude),
                               altitude_buffer, ARRAY_SIZE(altitude_buffer));

    buffer.AppendFormat(_T("%s %s, "), altitude_buffer, _("AGL"));
  }

  FormatUserAltitude(fixed(item.reach.direct),
                     altitude_buffer, ARRAY_SIZE(altitude_buffer));

  buffer.AppendFormat(_T("%s %s"), altitude_buffer, _("MSL"));

  // Draw title row

  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + text_padding, rc, buffer);

  // Format comment row

  if (reach_relevant) {
    buffer.Format(_T("%s: "), _("around terrain"));

    if (elevation_available) {
      RoughAltitude relative_arrival_altitude =
          item.reach.terrain - item.elevation;

      FormatRelativeUserAltitude(fixed((short)relative_arrival_altitude),
                                 altitude_buffer, ARRAY_SIZE(altitude_buffer));

     buffer.AppendFormat(_T("%s %s, "), altitude_buffer, _("AGL"));
    }

    FormatUserAltitude(fixed(item.reach.terrain),
                       altitude_buffer, ARRAY_SIZE(altitude_buffer));

    buffer.AppendFormat(_T("%s %s, "), altitude_buffer, _("MSL"));
  } else if (elevation_available &&
             (int)item.reach.direct >= (int)item.elevation &&
             item.reach.terrain_valid == ReachResult::Validity::UNREACHABLE) {
    buffer.UnsafeFormat(_T("%s "), _("Unreachable due to terrain."));
  } else {
    buffer.clear();
  }

  buffer += _("Arrival altitude incl. safety height");

  // Draw comment row

  canvas.Select(small_font);
  canvas.DrawClippedText(left,
                         rc.top + name_font.GetHeight() + 2 * text_padding,
                         rc, buffer);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const SelfMapItem &item,
                          const DialogLook &dialog_look,
                          const AircraftLook &look,
                          const MapSettings &settings)
{
  const unsigned line_height = rc.bottom - rc.top;
  const unsigned text_padding = Layout::GetTextPadding();

  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  int left = rc.left + line_height + text_padding;
  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + text_padding, rc,
                         _("Your Position"));

  TCHAR buffer[128];
  FormatGeoPoint(item.location, buffer, 128);

  canvas.Select(small_font);
  canvas.DrawClippedText(left,
                         rc.top + name_font.GetHeight() + 2 * text_padding,
                         rc, buffer);

  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };
  AircraftRenderer::Draw(canvas, settings, look, item.bearing, pt);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const AirspaceMapItem &item,
                          const DialogLook &dialog_look,
                          const AirspaceLook &look,
                          const AirspaceRendererSettings &renderer_settings)
{
  AirspaceListRenderer::Draw(canvas, rc, *item.airspace, dialog_look, look,
                             renderer_settings);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const WaypointMapItem &item,
                          const DialogLook &dialog_look,
                          const WaypointLook &look,
                          const WaypointRendererSettings &renderer_settings)
{
  WaypointListRenderer::Draw(canvas, rc, item.waypoint,
                             dialog_look, look, renderer_settings);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const MarkerMapItem &item,
                          const DialogLook &dialog_look,
                          const MarkerLook &look)
{
  const unsigned line_height = rc.bottom - rc.top;
  const unsigned text_padding = Layout::GetTextPadding();

  const Marker &marker = item.marker;

  const RasterPoint pt(rc.left + line_height / 2,
                       rc.top + line_height / 2);

  look.icon.Draw(canvas, pt);

  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  int left = rc.left + line_height + text_padding;

  StaticString<256> buffer;
  buffer.Format(_T("%s #%d"), _("Marker"), item.id + 1);
  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + text_padding, rc, buffer);

  TCHAR time_buffer[32], timespan_buffer[32];
  FormatSignedTimeHHMM(time_buffer, TimeLocal(marker.time.GetSecondOfDay()));
  FormatTimespanSmart(timespan_buffer, BrokenDateTime::NowUTC() - marker.time);
  buffer.Format(_("dropped %s ago"), timespan_buffer);
  buffer.AppendFormat(_T(" (%s)"), time_buffer);
  canvas.Select(small_font);
  canvas.DrawClippedText(left,
                         rc.top + name_font.GetHeight() + 2 * text_padding,
                         rc, buffer);
}

#ifdef HAVE_NOAA
void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const WeatherStationMapItem &item,
                          const DialogLook &dialog_look,
                          const NOAALook &look)
{
  const NOAAStore::Item &station = *item.station;
  NOAAListRenderer::Draw(canvas, rc, station, look, dialog_look);
}
#endif

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const ThermalMapItem &item,
                          const DialogLook &dialog_look,
                          const MapLook &look)
{
  const unsigned line_height = rc.bottom - rc.top;
  const unsigned text_padding = Layout::GetTextPadding();

  const ThermalSource &thermal = item.thermal;

  const RasterPoint pt(rc.left + line_height / 2,
                       rc.top + line_height / 2);

  look.thermal_source_icon.Draw(canvas, pt);

  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  int left = rc.left + line_height + text_padding;

  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + text_padding,
                         rc, _("Thermal"));

  StaticString<256> buffer;
  TCHAR lift_buffer[32], time_buffer[32], timespan_buffer[32];
  FormatUserVerticalSpeed(thermal.lift_rate, lift_buffer, 32);
  FormatSignedTimeHHMM(time_buffer, TimeLocal((int)thermal.time));

  int timespan = BrokenDateTime::NowUTC().GetSecondOfDay() - (int)thermal.time;
  if (timespan < 0)
    timespan += 24 * 60 * 60;

  FormatTimespanSmart(timespan_buffer, timespan);

  buffer.Format(_T("%s: %s"), _("Avg. lift"), lift_buffer);
  buffer.append(_T(" - "));
  buffer.AppendFormat(_("left %s ago"), timespan_buffer);
  buffer.AppendFormat(_T(" (%s)"), time_buffer);
  canvas.Select(small_font);
  canvas.DrawClippedText(left,
                         rc.top + name_font.GetHeight() + 2 * text_padding,
                         rc, buffer);
}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const TaskOZMapItem &item,
                          const DialogLook &dialog_look,
                          const TaskLook &look, const AirspaceLook &airspace_look,
                          const AirspaceRendererSettings &airspace_settings)
{
  const unsigned line_height = rc.bottom - rc.top;
  const unsigned text_padding = Layout::GetTextPadding();

  const ObservationZonePoint &oz = *item.oz;
  const Waypoint &waypoint = item.waypoint;

  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;

  TCHAR buffer[256];

  // Y-Coordinate of the second row
  int top2 = rc.top + name_font.GetHeight() + 2 * text_padding;

  // Use small font for details
  canvas.Select(small_font);

  // Draw details line
  UPixelScalar left = rc.left + line_height + text_padding;
  OrderedTaskPointRadiusLabel(*item.oz, buffer);
  if (!StringIsEmpty(buffer))
    canvas.DrawClippedText(left, top2, rc.right - left, buffer);

  // Draw waypoint name
  canvas.Select(name_font);
  OrderedTaskPointLabel(item.tp_type, waypoint.name.c_str(),
                        item.index, buffer);
  canvas.DrawClippedText(left, rc.top + text_padding,
                         rc.right - left, buffer);

  const RasterPoint pt(rc.left + line_height / 2,
                       rc.top + line_height / 2);
  PixelScalar radius = std::min(PixelScalar(line_height / 2
                                            - 2 * text_padding),
                                Layout::FastScale(10));
  OZPreviewRenderer::Draw(canvas, oz, pt, radius, look,
                          airspace_settings, airspace_look);

}

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const TrafficMapItem &item,
                          const DialogLook &dialog_look,
                          const TrafficLook &traffic_look,
                          const TrafficList *traffic_list)
{
  const unsigned line_height = rc.bottom - rc.top;
  const unsigned text_padding = Layout::GetTextPadding();

  const FlarmTraffic *traffic = traffic_list == NULL ? NULL :
      traffic_list->FindTraffic(item.id);

  // Now render the text information
  const Font &name_font = *dialog_look.list.font_bold;
  const Font &small_font = *dialog_look.small_font;
  int left = rc.left + line_height + text_padding;

  const FlarmNetRecord *record = FlarmDetails::LookupRecord(item.id);

  StaticString<256> title_string;
  if (record && !StringIsEmpty(record->pilot))
    title_string = record->pilot.c_str();
  else
    title_string = _("FLARM Traffic");

  // Append name to the title, if it exists
  const TCHAR *callsign = FlarmDetails::LookupCallsign(item.id);
  if (callsign != NULL && !StringIsEmpty(callsign)) {
    title_string.append(_T(", "));
    title_string.append(callsign);
  }

  canvas.Select(name_font);
  canvas.DrawClippedText(left, rc.top + text_padding,
                         rc, title_string);

  StaticString<256> info_string;
  if (record && !StringIsEmpty(record->plane_type))
    info_string = record->plane_type;
  else if (traffic != NULL)
    info_string = FlarmTraffic::GetTypeString(traffic->type);
  else
    info_string = _("Unknown");

  // Generate the line of info about the target, if it's available
  if (traffic != NULL) {
    if (traffic->altitude_available) {
      TCHAR tmp[15];
      FormatUserAltitude(traffic->altitude, tmp, 15);
      info_string.AppendFormat(_T(", %s: %s"), _("Altitude"), tmp);
    }
    if (traffic->climb_rate_avg30s_available) {
      TCHAR tmp[15];
      FormatUserVerticalSpeed(traffic->climb_rate_avg30s, tmp, 15);
      info_string.AppendFormat(_T(", %s: %s"), _("Vario"), tmp);
    }
  }
  canvas.Select(small_font);
  canvas.DrawClippedText(left,
                         rc.top + name_font.GetHeight() + 2 * text_padding,
                         rc, info_string);

  RasterPoint pt = { (PixelScalar)(rc.left + line_height / 2),
                     (PixelScalar)(rc.top + line_height / 2) };

  // Render the representation of the traffic icon
  if (traffic != NULL)
    TrafficRenderer::Draw(canvas, traffic_look, *traffic, traffic->track,
                          item.color, pt);
}

#ifdef HAVE_SKYLINES_TRACKING_HANDLER

static void
Draw(Canvas &canvas, const PixelRect rc,
     const SkyLinesTrafficMapItem &item,
     const DialogLook &dialog_look)
{
  const Font &name_font = *dialog_look.list.font_bold;

  const unsigned line_height = rc.bottom - rc.top;
  const unsigned text_padding = Layout::GetTextPadding();
  const int left = rc.left + line_height + text_padding;
  const int top = rc.top + text_padding;

  StaticString<64> tmp;
  tmp.UnsafeFormat(_T("SkyLines %u"), item.id);

  canvas.Select(name_font);
  canvas.DrawText(left, top, tmp);
}

#endif /* HAVE_SKYLINES_TRACKING_HANDLER */

void
MapItemListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                          const MapItem &item,
                          const DialogLook &dialog_look, const MapLook &look,
                          const TrafficLook &traffic_look,
                          const FinalGlideBarLook &final_glide_look,
                          const MapSettings &settings,
                          const TrafficList *traffic_list)
{
  switch (item.type) {
  case MapItem::LOCATION:
    Draw(canvas, rc, (const LocationMapItem &)item, dialog_look);
    break;
  case MapItem::ARRIVAL_ALTITUDE:
    Draw(canvas, rc, (const ArrivalAltitudeMapItem &)item,
         dialog_look, final_glide_look);
    break;
  case MapItem::SELF:
    Draw(canvas, rc, (const SelfMapItem &)item,
         dialog_look, look.aircraft, settings);
    break;
  case MapItem::AIRSPACE:
    Draw(canvas, rc, (const AirspaceMapItem &)item,
         dialog_look, look.airspace,
         settings.airspace);
    break;
  case MapItem::WAYPOINT:
    Draw(canvas, rc, (const WaypointMapItem &)item,
         dialog_look, look.waypoint,
         settings.waypoint);
    break;
  case MapItem::TASK_OZ:
    Draw(canvas, rc, (const TaskOZMapItem &)item,
         dialog_look, look.task, look.airspace,
         settings.airspace);
    break;
  case MapItem::MARKER:
    Draw(canvas, rc, (const MarkerMapItem &)item, dialog_look, look.marker);
    break;

#ifdef HAVE_NOAA
  case MapItem::WEATHER:
    Draw(canvas, rc, (const WeatherStationMapItem &)item, dialog_look, look.noaa);
    break;
#endif

  case MapItem::TRAFFIC:
    Draw(canvas, rc, (const TrafficMapItem &)item,
         dialog_look, traffic_look, traffic_list);
    break;

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  case MapItem::SKYLINES_TRAFFIC:
    ::Draw(canvas, rc, (const SkyLinesTrafficMapItem &)item, dialog_look);
    break;
#endif

  case MapItem::THERMAL:
    Draw(canvas, rc, (const ThermalMapItem &)item, dialog_look, look);
    break;
  }
}
