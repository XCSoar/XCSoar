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

#include "GlueMapWindow.hpp"
#include "Dialogs/MapItemListDialog.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "MapWindow/MapItemList.hpp"
#include "MapWindow/MapItemListBuilder.hpp"

bool
GlueMapWindow::ShowMapItems(const GeoPoint &location)
{
  MapItemList list;
  MapItemListBuilder builder(list, location);

  fixed range = visible_projection.DistancePixelsToMeters(Layout::Scale(10));

  if (Basic().location_available)
    builder.AddSelfIfNear(Basic().location, Calculated().heading, range);

  const Airspaces *airspace_database = airspace_renderer.GetAirspaces();
  if (airspace_database)
    builder.AddVisibleAirspace(*airspace_database,
                               airspace_renderer.GetAirspaceWarnings(),
                               SettingsComputer().airspace,
                               SettingsMap().airspace, Basic(),
                               Calculated());

  if (way_points)
    builder.AddWaypoints(*way_points, range);

  // Sort the list of map items
  list.Sort();

  // Show the list dialog
  ShowMapItemListDialog(*(SingleWindow *)get_root_owner(), list,
                        aircraft_look,
                        airspace_renderer.GetLook(),
                        way_point_renderer.GetLook(),
                        SettingsMap());

  // Save function result for later
  return !list.empty();
}
