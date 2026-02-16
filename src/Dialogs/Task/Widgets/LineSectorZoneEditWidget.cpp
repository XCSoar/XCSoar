// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LineSectorZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Language/Language.hpp"

enum Controls {
  LENGTH,
};

LineSectorZoneEditWidget::LineSectorZoneEditWidget(LineSectorZone &_oz,
                                                   bool _length_editable) noexcept
  :ObservationZoneEditWidget(_oz),
   length_editable(_length_editable) {}

void
LineSectorZoneEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  ObservationZoneEditWidget::Prepare(parent, rc);

  AddFloat(_("Gate width"), _("Width of the start/finish gate."),
           "%.1f %s", "%.1f",
           0.1, 200, 1, true,
           UnitGroup::DISTANCE, GetObject().GetLength(),
           this);
  SetRowEnabled(LENGTH, length_editable);
}

bool
LineSectorZoneEditWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  if (length_editable) {
    auto length = GetObject().GetLength();
    if (SaveValue(LENGTH, UnitGroup::DISTANCE, length)) {
      GetObject().SetLength(length);
      changed = true;
    }
  }

  _changed |= changed;
  return true;
}
