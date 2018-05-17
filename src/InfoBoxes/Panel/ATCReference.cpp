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

#include "ATCReference.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/ActionListener.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Util/Macros.hpp"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"

enum Controls {
  WAYPOINT,
  LOCATION,
  RELOCATE,
  CLEAR,
};

class ATCReferencePanel : public RowFormWidget, ActionListener {
public:
  ATCReferencePanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void UpdateValues();

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
ATCReferencePanel::UpdateValues()
{
  const GeoPoint &location =
    CommonInterface::GetComputerSettings().poi.atc_reference;

  const auto waypoint = location.IsValid()
    ? way_points.GetNearest(location, 100)
    : nullptr;

  SetText(WAYPOINT, waypoint != nullptr ? waypoint->name.c_str() : _T("---"));

  const TCHAR *location_string;
  TCHAR buffer[64];
  if (location.IsValid()) {
    FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer),
                   CommonInterface::GetUISettings().format.coordinate_format);
    location_string = buffer;
  } else
    location_string = _T("---");

  SetText(LOCATION, location_string);
}

void
ATCReferencePanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  AddReadOnly(_("Waypoint"));
  AddReadOnly(_("Location"));

  AddButton(_("Relocate"), *this, RELOCATE);
  AddButton(_("Clear"), *this, CLEAR);

  UpdateValues();
}

void
ATCReferencePanel::OnAction(int id)
{
  GeoPoint &location =
    CommonInterface::SetComputerSettings().poi.atc_reference;

  switch (id) {
  case RELOCATE: {
    auto waypoint = ShowWaypointListDialog(CommonInterface::Basic().location);
    if (waypoint != nullptr) {
      location = waypoint->location;
      UpdateValues();
    }
  }
    break;

  case CLEAR:
    location.SetInvalid();
    UpdateValues();
    break;
  }
}

Widget *
LoadATCReferencePanel(unsigned id)
{
  return new ATCReferencePanel();
}
