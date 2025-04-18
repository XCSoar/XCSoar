// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapItemListSettingsPanel.hpp"
#include "Profile/Keys.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

MapItemListSettingsPanel::MapItemListSettingsPanel() noexcept
  :RowFormWidget(UIGlobals::GetDialogLook()) {}

void
MapItemListSettingsPanel::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
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
MapItemListSettingsPanel::Save(bool &changed) noexcept
{
  MapSettings &settings = CommonInterface::SetMapSettings();

  changed |= SaveValue(AddLocation, ProfileKeys::EnableLocationMapItem,
                       settings.item_list.add_location);

  changed |= SaveValue(AddArrivalAltitude, ProfileKeys::EnableArrivalAltitudeMapItem,
                       settings.item_list.add_arrival_altitude);

  return true;
}
