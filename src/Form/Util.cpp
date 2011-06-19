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
#include "DataField/Base.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Float.hpp"
#include "Profile/Profile.hpp"

#include <assert.h>

void
ShowFormControl(WndForm &form, const TCHAR *control_name, bool visible)
{
  Window *window = form.FindByName(control_name);
  assert(window != NULL);
  window->set_visible(visible);
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, bool value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  DataFieldBoolean &df = *(DataFieldBoolean *)ctl->GetDataField();
  df.Set(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, int value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->SetAsInteger(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, unsigned int value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  ctl->GetDataField()->SetAsInteger(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  df.Set(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name,
                 UnitGroup_t unit_group, int value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  Units_t unit = Units::GetUserUnitByGroup(unit_group);
  DataField *df = ctl->GetDataField();
  df->SetUnits(Units::GetUnitName(unit));
  df->SetAsInteger(iround(Units::ToUserUnit(fixed(value), unit)));
  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name,
                 UnitGroup_t unit_group, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  Units_t unit = Units::GetUserUnitByGroup(unit_group);

  DataFieldFloat *df = (DataFieldFloat *)ctl->GetDataField();
  df->SetUnits(Units::GetUnitName(unit));
  df->SetAsFloat(Units::ToUserUnit(value, unit));
  ctl->RefreshDisplay();
}

int
GetFormValueInteger(const WndForm &form, const TCHAR *control_name)
{
  assert(control_name != NULL);

  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  return control->GetDataField()->GetAsInteger();
}

fixed
GetFormValueFixed(const WndForm &form, const TCHAR *control_name)
{
  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  const DataFieldFloat &df = *(const DataFieldFloat *)control->GetDataField();
  return df.GetAsFixed();
}

bool
GetFormValueBoolean(const WndForm &form, const TCHAR *control_name)
{
  assert(control_name != NULL);

  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  const DataFieldBoolean &df =
    *(const DataFieldBoolean *)control->GetDataField();
  return df.GetAsBoolean();
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, bool &value)
{
  bool new_value = GetFormValueBoolean(form, field);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
SaveFormPropertyNegated(const WndForm &form, const TCHAR *field,
                        const TCHAR *profile_key, bool &value)
{
  bool new_value = !GetFormValueBoolean(form, field);
  if (new_value == value)
    return false;

  value = new_value;
  Profile::Set(profile_key, value);
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, unsigned int &value)
{
  unsigned new_value = (unsigned)GetFormValueInteger(form, field);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, int &value)
{
  int new_value = GetFormValueInteger(form, field);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, short &value)
{
  short new_value = (short)GetFormValueInteger(form, field);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
SaveFormProperty(WndForm &form, const TCHAR *control_name, fixed &value)
{
  fixed new_value = GetFormValueFixed(form, control_name);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

#ifdef FIXED_MATH
bool
SaveFormProperty(WndForm &form, const TCHAR *control_name, double &value)
{
  double new_value = (double)GetFormValueFixed(form, control_name);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}
#endif

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 bool &value, const TCHAR *registry_name)
{
  assert(control_name != NULL);
  assert(registry_name != NULL);

  if (!SaveFormProperty(form, control_name, value))
    return false;

  Profile::Set(registry_name, value);
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, const TCHAR *reg,
                 bool &value)
{
  if (SaveFormProperty(form, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, const TCHAR *reg,
                 unsigned int &value)
{
  if (SaveFormProperty(form, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, const TCHAR *reg,
                 int &value)
{
  if (SaveFormProperty(form, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, const TCHAR *reg,
                 short &value)
{
  if (SaveFormProperty(form, field, value)) {
    Profile::Set(reg, value);
    return true;
  } else {
    return false;
  }
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 UnitGroup_t unit_group, int &value,
                 const TCHAR *registry_name)
{
  assert(control_name != NULL);
  assert(registry_name != NULL);

  Units_t unit = Units::GetUserUnitByGroup(unit_group);
  int new_value = GetFormValueInteger(form, control_name);
  new_value = iround(Units::ToSysUnit(fixed(new_value), unit));
  if (new_value == value)
    return false;

  value = new_value;
  Profile::Set(registry_name, new_value);
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 UnitGroup_t unit_group, unsigned &value,
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
                 UnitGroup_t unit_group, fixed &value)
{
  assert(control_name != NULL);

  fixed new_value = GetFormValueFixed(form, control_name);
  Units_t unit = Units::GetUserUnitByGroup(unit_group);
  new_value = Units::ToSysUnit(new_value, unit);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 UnitGroup_t unit_group, fixed &value,
                 const TCHAR *registry_name)
{
  assert(registry_name != NULL);

  if (!SaveFormProperty(form, control_name, unit_group, value))
    return false;

  Profile::Set(registry_name, value);
  return true;
}
