/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "VariableKeyholeZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Language/Language.hpp"

enum Controls
  {
  RADIUS,
  INNER_RADIUS,
  START_RADIAL,
  END_RADIAL,
  };

//------------------------------------------------------------------------------
VariableKeyholeZoneEditWidget::VariableKeyholeZoneEditWidget(VariableKeyholeZone &_oz)
  :ObservationZoneEditWidget(_oz) {}

//------------------------------------------------------------------------------
void
VariableKeyholeZoneEditWidget::Prepare(ContainerWindow &parent,
                                         const PixelRect &rc)
  {
  ObservationZoneEditWidget::Prepare(parent, rc);

  this->AddFloat(_("Radius"),
                 _("Radius of the OZ sector."),
                 _T("%.1f %s"),
                 _T("%.1f"),
                 0.1,
                 200,
                 1,
                 true,
                 UnitGroup::DISTANCE,
                 GetObject().GetRadius(),
                 this);

  this->AddFloat(_("Inner radius"),
                 _("Inner radius of the OZ sector."),
                 _T("%.1f %s"),
                 _T("%.1f"),
                 0.1,
                 100,
                 1,
                 true,
                 UnitGroup::DISTANCE,
                 GetObject().GetInnerRadius(),
                 this);

  this->AddAngle(_("Start radial"),
                 _("The most CCW radial of the OZ area"),
                 this->GetObject().GetStartRadial(),
                 10,
                 true,
                 this);

  this->AddAngle(_("Finish radial"),
                 _("The most CW radial of the OZ area"),
                 this->GetObject().GetEndRadial(),
                 10,
                 true,
                 this);
  }

//------------------------------------------------------------------------------
bool
VariableKeyholeZoneEditWidget::Save(bool &_changed)
  {
  bool changed = false;

  auto radius = GetObject().GetRadius();
  if (SaveValue(RADIUS, UnitGroup::DISTANCE, radius))
    {
    this->GetObject().SetRadius(radius);
    changed = true;
    }

  auto inner_radius = GetObject().GetInnerRadius();
  if (SaveValue(INNER_RADIUS, UnitGroup::DISTANCE, inner_radius))
    {
    this->GetObject().SetInnerRadius(inner_radius);
    changed = true;
    }

  Angle angle = GetObject().GetStartRadial();
  if (SaveValue(START_RADIAL, angle))
    {
    this->GetObject().SetStartRadial(angle);
    changed = true;
    }
  angle = GetObject().GetEndRadial();
  if (SaveValue(END_RADIAL, angle))
    {
    this->GetObject().SetEndRadial(angle);
    changed = true;
    }

  _changed |= changed;
  return true;
  }
