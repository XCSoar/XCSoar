/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Form/Util.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "DataField/Float.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"

#include <assert.h>

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != nullptr);

  Unit unit = Units::GetUserUnitByGroup(unit_group);

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.Set(Units::ToUserUnit(value, unit));
  ctl->RefreshDisplay();
}

void
LoadOptionalFormProperty(SubForm &form, const TCHAR *control_name,
                         UnitGroup unit_group, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  Unit unit = Units::GetUserUnitByGroup(unit_group);
  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.Set(Units::ToUserUnit(value, unit));
  ctl->RefreshDisplay();
}
