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
  :RowFormWidget(UIGlobals::GetDialogLook()) {}

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
}

bool
MapItemListSettingsPanel::Save(bool &changed)
{
  MapSettings &settings = CommonInterface::SetMapSettings();

  changed |= SaveValue(AddLocation, ProfileKeys::EnableLocationMapItem,
                       settings.item_list.add_location);

  changed |= SaveValue(AddArrivalAltitude, ProfileKeys::EnableArrivalAltitudeMapItem,
                       settings.item_list.add_arrival_altitude);

  return true;
}
