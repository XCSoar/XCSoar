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

#include "LineSectorZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Language/Language.hpp"

enum Controls {
  LENGTH,
};

LineSectorZoneEditWidget::LineSectorZoneEditWidget(LineSectorZone &_oz,
                                                   bool _length_editable)
  :ObservationZoneEditWidget(_oz),
   length_editable(_length_editable) {}

void
LineSectorZoneEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  ObservationZoneEditWidget::Prepare(parent, rc);

  AddFloat(_("Gate width"), _("Width of the start/finish gate."),
           _T("%.1f %s"), _T("%.1f"),
           0.1, 200, 1, true,
           UnitGroup::DISTANCE, GetObject().GetLength(),
           this);
  SetRowEnabled(LENGTH, length_editable);
}

bool
LineSectorZoneEditWidget::Save(bool &_changed)
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
