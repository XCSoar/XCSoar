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

#include "Dialogs/MapItemListDialog.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/Airspace.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Language/Language.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Look/WaypointLook.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Units/UnitsFormatter.hpp"
#include "MapWindow/MapItem.hpp"
#include "MapWindow/MapItemList.hpp"
#include "MapWindow/MapItemListBuilder.hpp"

#include <algorithm>
#include <cstdio>

static const AirspaceLook *airspace_look;
static const AirspaceRendererSettings *airspace_renderer_settings;
static const WaypointLook *waypoint_look;
static const WaypointRendererSettings *waypoint_renderer_settings;
static const MapItemList *list;

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  const unsigned line_height = rc.bottom - rc.top;

  const MapItem &map_item = *(*list)[idx];
  if (map_item.type == MapItem::AIRSPACE) {
    const AirspaceMapItem &airspace_map_item = (const AirspaceMapItem &)map_item;
    const AbstractAirspace &airspace = *airspace_map_item.airspace;

    RasterPoint pt = { rc.left + line_height / 2,
                       rc.top + line_height / 2};
    unsigned radius = std::min(line_height / 2  - Layout::FastScale(4),
                               (unsigned)Layout::FastScale(10));
    AirspacePreviewRenderer::Draw(canvas, airspace, pt, radius,
                                  *airspace_renderer_settings, *airspace_look);

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
  } else if (map_item.type == MapItem::WAYPOINT) {
    const WaypointMapItem &waypoint_map_item = (const WaypointMapItem &)map_item;
    const Waypoint &waypoint = waypoint_map_item.waypoint;

    RasterPoint pt = { rc.left + line_height / 2,
                       rc.top + line_height / 2};
    WaypointIconRenderer wir(*waypoint_renderer_settings, *waypoint_look,
                             canvas);
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
}

static void
ShowMapItemDialog(const MapItem &item, SingleWindow &parent)
{
  if (item.type == MapItem::AIRSPACE)
    dlgAirspaceDetails(*((const AirspaceMapItem &)item).airspace);
  else if (item.type == MapItem::WAYPOINT)
    dlgWaypointDetailsShowModal(parent,
                                ((const WaypointMapItem &)item).waypoint);
}

static void
ShowMapItemListDialog(SingleWindow &parent,
                      const MapItemList &_list,
                      const AirspaceLook &_airspace_look,
                      const AirspaceRendererSettings &airspace_settings,
                      const WaypointLook &_waypoint_look,
                      const WaypointRendererSettings &waypoint_settings)
{
  if (_list.empty())
    return;

  switch (_list.size()) {
  case 0:
    /* no (visible) airspace here */
    assert(false);
    break;

  case 1:
    /* only one airspace, show it */
    ShowMapItemDialog(*_list[0], parent);
    break;

  default:
    /* more than one airspace: show a list */
    unsigned line_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                           Fonts::MapLabel.get_height();
    list = &_list;

    airspace_renderer_settings = &airspace_settings;
    airspace_look = &_airspace_look;
    waypoint_renderer_settings = &waypoint_settings;
    waypoint_look = &_waypoint_look;

    int i = ListPicker(parent, _("Map elements at this location"),
                       _list.size(), 0, line_height, PaintListItem);
    assert(i >= -1 && i < (int)_list.size());
    if (i >= 0)
      ShowMapItemDialog(*_list[i], parent);
  }
}

bool
ShowMapItemListDialog(SingleWindow &parent, const GeoPoint &location,
                          const AirspaceRenderer &renderer,
                          const AirspaceComputerSettings &computer_settings,
                          const AirspaceRendererSettings &renderer_settings,
                          const Waypoints *waypoints,
                          const WaypointLook &_waypoint_look,
                          const WaypointRendererSettings &waypoint_settings,
                          const MoreData &basic, const DerivedInfo &calculated,
                          fixed range)
{
  MapItemList list;
  MapItemListBuilder builder(list, location);

  const Airspaces *airspace_database = renderer.GetAirspaces();
  if (airspace_database)
    builder.AddVisibleAirspace(*airspace_database,
                               renderer.GetAirspaceWarnings(),
                               computer_settings, renderer_settings, basic,
                               calculated);

  if (waypoints)
    builder.AddWaypoints(*waypoints, range);

  // Sort the list of map items
  list.Sort();

  // Show the list dialog
  ShowMapItemListDialog(parent, list, renderer.GetLook(), renderer_settings,
                        _waypoint_look, waypoint_settings);

  // Save function result for later
  return !list.empty();
}
