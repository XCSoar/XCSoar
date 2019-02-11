/*
 Copyright_License {

 XCSoar Glide Computer - http://www.xcsoar.org/
 Copyright (C) 2000-2016 The XCSoar Project
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

#include "MapItemListSettingsPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

MapItemListSettingsPanel::MapItemListSettingsPanel()
    : RowFormWidget(UIGlobals::GetDialogLook())
{
}

void
MapItemListSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const MapSettings &settings = CommonInterface::GetMapSettings();

  AddBoolean(_("Show Location row"),
             _("If enabled a row at the top will be added showing you the "
               "distance and bearing to the location and the elevation."),
             settings.item_list.add_location);

  AddBoolean(_("Show Arrival Altitude"),
             _("If enabled a row at the top will be added showing you the "
               "arrival altitude at the location."),
             settings.item_list.add_arrival_altitude);

  AddBoolean(_("Show Airspace"), _("Airspace are listed"),
             settings.item_list.add_airspace);

  AddBoolean(_("Show Traffic"), _("Flarm-traffic items are listed."),
             settings.item_list.add_traffic);

  AddInteger(
      _("Hit Range on the Screen"),
      _("Lists more map items with a radius in% of screen width around the center of the screen. "
        "Useful for pure key control. 0% defines the default behavior, "
        "at 100% a radius is taken into account until the next screen edge. "
        "125% is to be used to list all displayed map items on the screen. "
        "A maximum radius of 50km radius is calculated. A mouse click determines the center point."),
      _("%d %%"), _("%d %%"), 0, 125, 25,
      settings.item_list.range_of_nearest_map_items_in_percent_of_displaysize);

  AddInteger(
      _("Only Landable on Hit Range>"),
      _("As the zoom level decreases, the calculated range of hits increases in kilometers. "
        "From this limit only landable will be listed."),
      _("%d km"), _("%d km"), 0, 50, 10,
      settings.item_list.rangefilter_all_waypoint_up_to_km);
}

bool
MapItemListSettingsPanel::Save(bool &changed)
{
  MapSettings &settings = CommonInterface::SetMapSettings();

  changed |= SaveValue(AddLocation, ProfileKeys::EnableLocationMapItem,
                       settings.item_list.add_location);

  changed |= SaveValue(AddArrivalAltitude,
                       ProfileKeys::EnableArrivalAltitudeMapItem,
                       settings.item_list.add_arrival_altitude);

  changed |= SaveValue(AddAirspace, ProfileKeys::EnableAirspaceMapItem,
                       settings.item_list.add_airspace);

  changed |= SaveValue(AddTraffic, ProfileKeys::EnableTrafficMapItem,
                       settings.item_list.add_traffic);

  changed |= SaveValue(
      RangeOfNearestMapItemsInPercentOfDisplaysize,
      ProfileKeys::RangeOfNearestMapItemsInPercent,
      settings.item_list.range_of_nearest_map_items_in_percent_of_displaysize);

  changed |= SaveValue(RangefilterAllWaypoint,
                       ProfileKeys::RangefilterAllWaypointUpToKm,
                       settings.item_list.rangefilter_all_waypoint_up_to_km);

  return true;
}
