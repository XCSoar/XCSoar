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
#include "Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Language/Language.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "MapWindow/MapItem.hpp"
#include "MapWindow/MapItemList.hpp"
#include "MapWindow/MapItemListBuilder.hpp"
#include "Renderer/MapItemListRenderer.hpp"

static const AirspaceLook *airspace_look;
static const AirspaceRendererSettings *airspace_renderer_settings;
static const WaypointLook *waypoint_look;
static const WaypointRendererSettings *waypoint_renderer_settings;
static const MapItemList *list;

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  const MapItem &item = *(*list)[idx];
  MapItemListRenderer::Draw(canvas, rc, item,
                            *airspace_look, *airspace_renderer_settings,
                            *waypoint_look, *waypoint_renderer_settings);
}

void
ShowMapItemDialog(const MapItem &item, SingleWindow &parent)
{
  switch (item.type) {
  case MapItem::AIRSPACE:
    dlgAirspaceDetails(*((const AirspaceMapItem &)item).airspace);
    break;
  case MapItem::WAYPOINT:
    dlgWaypointDetailsShowModal(parent,
                                ((const WaypointMapItem &)item).waypoint);
    break;
  }
}

void
ShowMapItemListDialog(SingleWindow &parent,
                      const MapItemList &_list,
                      const AirspaceLook &_airspace_look,
                      const AirspaceRendererSettings &airspace_settings,
                      const WaypointLook &_waypoint_look,
                      const WaypointRendererSettings &waypoint_settings)
{
  switch (_list.size()) {
  case 0:
    /* no map items in the list */
    return;

  case 1:
    /* only one map item, show it */
    ShowMapItemDialog(*_list[0], parent);
    break;

  default:
    /* more than one map item: show a list */
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
