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

#include "SectorZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/SectorZone.hpp"
#include "Engine/Task/ObservationZones/AnnularSectorZone.hpp"
#include "Language/Language.hpp"

enum Controls {
  RADIUS,
  START_RADIAL,
  END_RADIAL,
  INNER_RADIUS,
};

SectorZoneEditWidget::SectorZoneEditWidget(SectorZone &_oz)
  :ObservationZoneEditWidget(_oz) {}

void
SectorZoneEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  ObservationZoneEditWidget::Prepare(parent, rc);

  const auto shape = GetObject().GetShape();

  AddFloat(_("Radius"), _("Radius of the OZ sector."),
           _T("%.1f %s"), _T("%.1f"),
           0.1, 200, 1, true,
           UnitGroup::DISTANCE, GetObject().GetRadius(),
           this);

  if (shape == ObservationZone::Shape::SYMMETRIC_QUADRANT) {
    AddDummy();
    AddDummy();
  } else {
    AddAngle(_("Start radial"), _("Start radial of the OZ area"),
             GetObject().GetStartRadial(), 10, true,
             this);

    AddAngle(_("Finish radial"), _("Finish radial of the OZ area"),
             GetObject().GetEndRadial(), 10, true,
             this);
  }

  if (shape == ObservationZonePoint::Shape::ANNULAR_SECTOR) {
    const AnnularSectorZone &annulus = (const AnnularSectorZone &)GetObject();

    AddFloat(_("Inner radius"), _("Inner radius of the OZ sector."),
             _T("%.1f %s"), _T("%.1f"),
             0.1, 100, 1, true,
             UnitGroup::DISTANCE, annulus.GetInnerRadius(),
             this);
  }
}

bool
SectorZoneEditWidget::Save(bool &_changed)
{
  const auto shape = GetObject().GetShape();
  bool changed = false;

  auto radius = GetObject().GetRadius();
  if (SaveValue(RADIUS, UnitGroup::DISTANCE, radius)) {
    GetObject().SetRadius(radius);
    changed = true;
  }

  if (shape == ObservationZone::Shape::SYMMETRIC_QUADRANT) {
  } else {
    Angle radial = GetObject().GetStartRadial();
    if (SaveValue(START_RADIAL, radial)) {
      GetObject().SetStartRadial(radial);
      changed = true;
    }

    radial = GetObject().GetEndRadial();
    if (SaveValue(END_RADIAL, radial)) {
      GetObject().SetEndRadial(radial);
      changed = true;
    }
  }

  if (GetObject().GetShape() == ObservationZonePoint::Shape::ANNULAR_SECTOR) {
    AnnularSectorZone &annulus = (AnnularSectorZone &)GetObject();

    radius = annulus.GetInnerRadius();
    if (SaveValue(INNER_RADIUS, UnitGroup::DISTANCE, radius)) {
      annulus.SetInnerRadius(radius);
      changed = true;
    }
  }

  _changed |= changed;
  return true;
}
