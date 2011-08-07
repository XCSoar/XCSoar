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
#include "DataField/Enum.hpp"
#include "DataField/String.hpp"
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
SetFormControlEnabled(WndForm &form, const TCHAR *control_name, bool enabled)
{
  Window *window = form.FindByName(control_name);
  assert(window != NULL);
  window->set_enabled(enabled);
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
LoadFormProperty(WndForm &form, const TCHAR *control_name,
                 const StaticEnumChoice *list, unsigned value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  DataFieldEnum &df = *(DataFieldEnum *)ctl->GetDataField();
  if (list[0].help != NULL)
    df.EnableItemHelp(true);

  df.AddChoices(list);
  df.SetAsInteger(value);

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

void
LoadFormProperty(WndForm &form, const TCHAR *control_name,
                 const TCHAR *value)
{
  assert(control_name != NULL);
  assert(value != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  DataFieldString *df = (DataFieldString *)ctl->GetDataField();
  assert(df != NULL);

  df->Set(value);
  ctl->RefreshDisplay();
}

void
LoadFormPropertyFromProfile(WndForm &form, const TCHAR *control_name,
                            const TCHAR *profile_key)
{
  TCHAR buffer[512];
  const TCHAR *value = Profile::Get(profile_key, buffer, 512)
    ? buffer
    : _T("");
  LoadFormProperty(form, control_name, value);
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

const TCHAR *
GetFormValueString(const WndForm &form, const TCHAR *control_name)
{
  assert(control_name != NULL);

  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  const DataFieldString *df = (const DataFieldString *)control->GetDataField();
  assert(df != NULL);

  return df->GetAsString();
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
SaveFormProperty(const WndForm &form, const TCHAR *field, uint8_t &value)
{
  uint8_t new_value = (uint8_t)GetFormValueInteger(form, field);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *field, uint16_t &value)
{
  uint16_t new_value = (uint16_t)GetFormValueInteger(form, field);
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
SaveFormProperty(const WndForm &form, const TCHAR *field, const TCHAR *reg,
                 uint8_t &value)
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
                 uint16_t &value)
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

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 TCHAR *buffer, size_t max_size)
{
  assert(max_size > 0);

  const TCHAR *value = GetFormValueString(form, control_name);
  assert(value != NULL);

  size_t length = _tcslen(value);
  if (length >= max_size)
    length = max_size - 1;

  if (_tcsncmp(value, buffer, length) == 0 && buffer[length] == _T('\0'))
    /* not modified */
    return false;

  std::copy(value, value + length, buffer);
  buffer[length] = _T('\0');
  return true;
}

bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 TCHAR *buffer, size_t max_size,
                 const TCHAR *profile_key)
{
  assert(max_size > 0);
  assert(profile_key != NULL);

  const TCHAR *value = GetFormValueString(form, control_name);
  assert(value != NULL);

  size_t length = _tcslen(value);
  if (length >= max_size)
    length = max_size - 1;

  if (_tcsncmp(value, buffer, length) == 0 && buffer[length] == _T('\0'))
    /* not modified */
    return false;

  std::copy(value, value + length, buffer);
  buffer[length] = _T('\0');
  Profile::Set(profile_key, buffer);
  return true;
}

bool
SaveFormPropertyToProfile(const WndForm &form, const TCHAR *control_name,
                          const TCHAR *profile_key)
{
  assert(profile_key != NULL);

  const TCHAR *value = GetFormValueString(form, control_name);
  assert(value != NULL);

  TCHAR buffer[512];
  const TCHAR *old = Profile::Get(profile_key, buffer, 512)
    ? buffer
    : _T("");

  if (_tcscmp(value, old) == 0)
    return false;

  Profile::Set(profile_key, value);
  return true;
}
