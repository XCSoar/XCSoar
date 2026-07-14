// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SectorZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/SectorZone.hpp"
#include "Engine/Task/ObservationZones/AnnularSectorZone.hpp"
#include "Engine/Task/ObservationZones/SymmetricSectorZone.hpp"
#include "Language/Language.hpp"

enum Controls {
  RADIUS,
  START_RADIAL,
  END_RADIAL,
  ANGLE,
  INNER_RADIUS,
};

SectorZoneEditWidget::SectorZoneEditWidget(SectorZone &_oz) noexcept
  :ObservationZoneEditWidget(_oz) {}

void
SectorZoneEditWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  ObservationZoneEditWidget::Prepare(parent, rc);

  const auto shape = GetObject().GetShape();

  AddFloat(_("Radius"), _("Radius of the OZ sector."),
           "%.1f %s", "%.1f",
           0.1, 200, 1, true,
           UnitGroup::DISTANCE, GetObject().GetRadius(),
           this);

  if (shape == ObservationZone::Shape::SYMMETRIC_SECTOR) {
    AddDummy(); // Start radial
    AddDummy(); // Finish radial

    if (const auto *oz = dynamic_cast<const SymmetricSectorZone *>(&GetObject())) {
      Angle angle = oz->GetSectorAngle();
      AddAngle(_("Angle"), _("Angle of the OZ sector"),
               angle, 10, true,
               this);
    } else {
      assert(false);
      AddDummy(); // Angle
    }
  } else {
    AddAngle(_("Start radial"), _("Start radial of the OZ area"),
             GetObject().GetStartRadial(), 10, true,
             this);

    AddAngle(_("Finish radial"), _("Finish radial of the OZ area"),
              GetObject().GetEndRadial(), 10, true,
              this);
    AddDummy(); // Angle
  }

  if (shape == ObservationZonePoint::Shape::ANNULAR_SECTOR) {
    const AnnularSectorZone &annulus = (const AnnularSectorZone &)GetObject();

    AddFloat(_("Inner radius"), _("Inner radius of the OZ sector."),
             "%.1f %s", "%.1f",
             0.1, 100, 1, true,
             UnitGroup::DISTANCE, annulus.GetInnerRadius(),
             this);
  } else {
    AddDummy(); // Inner radius
  }
}

bool
SectorZoneEditWidget::Save(bool &_changed) noexcept
{
  const auto shape = GetObject().GetShape();

  bool changed = false;

  auto radius = GetObject().GetRadius();
  if (SaveValue(RADIUS, UnitGroup::DISTANCE, radius)) {
    GetObject().SetRadius(radius);
    changed = true;
  }

  if (shape == ObservationZone::Shape::SYMMETRIC_SECTOR) {
    if (auto *oz = dynamic_cast<SymmetricSectorZone *>(&GetObject())) {
      Angle angle = Angle::Zero();
      if (SaveValue(ANGLE, angle)) {
        oz->SetSectorAngle(angle);
        changed = true;
      }
    } else {
      // Failed to cast to SymmetricSectorZone
      assert(false);
    }

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
