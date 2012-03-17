/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "GlueMapWindow.hpp"
#include "Look/MapLook.hpp"
#include "Dialogs/MapItemListDialog.hpp"
#include "UIGlobals.hpp"
#include "Screen/Layout.hpp"
#include "MapWindow/MapItemList.hpp"
#include "MapWindow/MapItemListBuilder.hpp"
#include "Computer/GlideComputer.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"

bool
GlueMapWindow::ShowMapItems(const GeoPoint &location,
                            bool show_empty_message) const
{
  fixed range = visible_projection.DistancePixelsToMeters(Layout::GetHitRadius());

  MapItemList list;
  MapItemListBuilder builder(list, location, range);

  builder.AddLocation(Basic(), terrain);

  if (route_planner)
    builder.AddArrivalAltitudes(*route_planner, terrain,
                                GetComputerSettings().task.safety_height_arrival);

  if (Basic().location_available)
    builder.AddSelfIfNear(Basic().location, Calculated().heading);

  if (task)
    builder.AddTaskOZs(*task);

  const Airspaces *airspace_database = airspace_renderer.GetAirspaces();
  if (airspace_database)
    builder.AddVisibleAirspace(*airspace_database,
                               airspace_renderer.GetAirspaceWarnings(),
                               GetComputerSettings().airspace,
                               GetMapSettings().airspace, Basic(),
                               Calculated());

  if (marks && render_projection.GetMapScale() <= fixed_int_constant(30000))
    builder.AddMarkers(*marks);

  if (render_projection.GetMapScale() <= fixed_int_constant(4000))
    builder.AddThermals(Calculated().thermal_locator, Basic(), Calculated());

  if (waypoints)
    builder.AddWaypoints(*waypoints);

  if (Basic().flarm.available)
    builder.AddTraffic(Basic().flarm);

  // Sort the list of map items
  list.Sort();

  // Show the list dialog
  if (list.empty()) {
    if (show_empty_message)
      MessageBoxX(_("There is nothing interesting near this location."),
                  _("Map elements at this location"), MB_OK | MB_ICONINFORMATION);

    return false;
  }

  ShowMapItemListDialog(UIGlobals::GetMainWindow(), list,
                        UIGlobals::GetDialogLook(), look, traffic_look,
                        final_glide_bar_renderer.GetLook(), GetMapSettings(),
                        glide_computer != NULL
                        ? &glide_computer->GetAirspaceWarnings() : NULL);
  return true;
}
