// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyholeZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Language/Language.hpp"

enum Controls {
  RADIUS,
  INNER_RADIUS,
  ANGLE,
};

KeyholeZoneEditWidget::KeyholeZoneEditWidget(KeyholeZone &_oz) noexcept
  :ObservationZoneEditWidget(_oz) {}

void
KeyholeZoneEditWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  ObservationZoneEditWidget::Prepare(parent, rc);

  AddFloat(_("Radius"), _("Radius of the OZ sector."),
           "%.1f %s", "%.1f",
           0.1, 200, 1, true,
           UnitGroup::DISTANCE, GetObject().GetRadius(),
           this);

  AddFloat(_("Inner radius"), _("Inner radius of the OZ sector."),
           "%.1f %s", "%.1f",
           0.1, 100, 1, true,
           UnitGroup::DISTANCE, GetObject().GetInnerRadius(),
           this);

  AddAngle(_("Angle"), nullptr,
           GetObject().GetSectorAngle(), 10, true,
           this);
}

bool
KeyholeZoneEditWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto radius = GetObject().GetRadius();
  if (SaveValue(RADIUS, UnitGroup::DISTANCE, radius)) {
    GetObject().SetRadius(radius);
    changed = true;
  }

  auto inner_radius = GetObject().GetInnerRadius();
  if (SaveValue(INNER_RADIUS, UnitGroup::DISTANCE, inner_radius)) {
    GetObject().SetInnerRadius(inner_radius);
    changed = true;
  }

  Angle angle = GetObject().GetSectorAngle();
  if (SaveValue(ANGLE, angle)) {
    GetObject().SetSectorAngle(angle);
    changed = true;
  }

  _changed |= changed;
  return true;
}
