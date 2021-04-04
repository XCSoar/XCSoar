/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "CylinderZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/CylinderZone.hpp"
#include "Language/Language.hpp"

enum Controls {
  RADIUS,
};

CylinderZoneEditWidget::CylinderZoneEditWidget(CylinderZone &_oz,
                                               bool _radius_editable) noexcept
  :ObservationZoneEditWidget(_oz),
   radius_editable(_radius_editable) {}

void
CylinderZoneEditWidget::Prepare(ContainerWindow &parent,
                                const PixelRect &rc) noexcept
{
  ObservationZoneEditWidget::Prepare(parent, rc);

  AddFloat(_("Radius"), _("Radius of the OZ cylinder."),
           _T("%.1f %s"), _T("%.1f"),
           0.1, 200, 1, true,
           UnitGroup::DISTANCE, GetObject().GetRadius(),
           this);
  SetRowEnabled(RADIUS, radius_editable);
}

bool
CylinderZoneEditWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  if (radius_editable) {
    auto radius = GetObject().GetRadius();
    if (SaveValue(RADIUS, UnitGroup::DISTANCE, radius)) {
      GetObject().SetRadius(radius);
      changed = true;
    }
  }

  _changed |= changed;
  return true;
}
