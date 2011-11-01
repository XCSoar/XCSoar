/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include <assert.h>

void
LoadFormProperty(WndForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, int value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  Unit unit = Units::GetUserUnitByGroup(unit_group);
  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.SetAsInteger(iround(Units::ToUserUnit(fixed(value), unit)));
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  Unit unit = Units::GetUserUnitByGroup(unit_group);

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.SetAsFloat(Units::ToUserUnit(value, unit));
  ctl->RefreshDisplay();
}

void
LoadOptionalFormProperty(WndForm &form, const TCHAR *control_name,
                         UnitGroup unit_group, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  Unit unit = Units::GetUserUnitByGroup(unit_group);
  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.SetUnits(Units::GetUnitName(unit));
  df.SetAsInteger(iround(Units::ToUserUnit(fixed(value), unit)));
  ctl->RefreshDisplay();
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, unsigned &value,
                 const TCHAR *registry_name)
{
  int value2 = value;
  if (SaveFormProperty(form, control_name, unit_group, value2,
                       registry_name)) {
    value = value2;
    return true;
  } else
    return false;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 UnitGroup unit_group, fixed &value)
{
  assert(control_name != NULL);

  fixed new_value = GetFormValueFixed(form, control_name);
  Unit unit = Units::GetUserUnitByGroup(unit_group);
  new_value = Units::ToSysUnit(new_value, unit);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}
