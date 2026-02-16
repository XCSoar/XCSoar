// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
           "%.1f %s", "%.1f",
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
