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

#include "RowFormWidget.hpp"
#include "Form/DataField/Float.hpp"
#include "Profile/Profile.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"

#include <assert.h>

void
RowFormWidget::AddReadOnly(const TCHAR *label, const TCHAR *help,
                           const TCHAR *display_format,
                           UnitGroup unit_group, fixed value)
{
  WndProperty *edit = Add(label, help, true);
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  value = Units::ToUserUnit(value, unit);
  DataFieldFloat *df = new DataFieldFloat(display_format, display_format,
                                          fixed(0), fixed(0),
                                          value, fixed(1), false);
  df->SetUnits(Units::GetUnitName(unit));
  edit->SetDataField(df);
}

WndProperty *
RowFormWidget::AddFloat(const TCHAR *label, const TCHAR *help,
                        const TCHAR *display_format,
                        const TCHAR *edit_format,
                        fixed min_value, fixed max_value,
                        fixed step, bool fine,
                        UnitGroup unit_group, fixed value,
                        DataField::DataAccessCallback callback)
{
  WndProperty *edit = Add(label, help);
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  value = Units::ToUserUnit(value, unit);
  DataFieldFloat *df = new DataFieldFloat(edit_format, display_format,
                                          min_value, max_value,
                                          value, step, fine, callback);
  df->SetUnits(Units::GetUnitName(unit));
  edit->SetDataField(df);
  return edit;
}

void
RowFormWidget::LoadValue(unsigned i, fixed value, UnitGroup unit_group)
{
  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  WndProperty &control = GetControl(i);
  DataFieldFloat &df = *(DataFieldFloat *)control.GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.Set(Units::ToUserUnit(value, unit));
  df.SetUnits(Units::GetUnitName(unit));
  control.RefreshDisplay();
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group, fixed &value) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);

  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  fixed new_value = df.GetAsFixed();
  fixed old_value = Units::ToUserUnit(value, unit);

  if (fabs(new_value - old_value) < df.GetStep() / 100)
    return false;

  value = Units::ToSysUnit(new_value, unit);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         const char *registry_key, fixed &value) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::REAL);

  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  fixed new_value = df.GetAsFixed();
  fixed old_value = Units::ToUserUnit(value, unit);

  if (fabs(new_value - old_value) < df.GetStep() / 100)
    return false;

  value = Units::ToSysUnit(new_value, unit);
  Profile::Set(registry_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, UnitGroup unit_group,
                         const char *registry_key, unsigned int &value) const
{
  const DataFieldFloat &df =
    (const DataFieldFloat &)GetDataField(i);
  assert(df.GetType() == DataField::Type::INTEGER ||
         df.GetType() == DataField::Type::REAL);

  const Unit unit = Units::GetUserUnitByGroup(unit_group);
  fixed new_value = df.GetAsFixed();
  fixed old_value = Units::ToUserUnit(fixed(value), unit);

  if (fabs(new_value - old_value) < df.GetStep() / 100)
    return false;

  value = iround(Units::ToSysUnit(new_value, unit));
  Profile::Set(registry_key, value);
  return true;
}
