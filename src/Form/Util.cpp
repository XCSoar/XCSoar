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

#include <assert.h>

void
ShowFormControl(WndForm &form, const TCHAR *control_name, bool visible)
{
  Window *window = form.FindByName(control_name);
  assert(window != NULL);
  window->set_visible(visible);
}

void
ShowOptionalFormControl(WndForm &form, const TCHAR *control_name,
                        bool visible)
{
  Window *window = form.FindByName(control_name);
  if (window != NULL)
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
SetFormValue(WndForm &form, const TCHAR *control_name, const TCHAR *value)
{
  assert(control_name != NULL);
  assert(value != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  ctl->SetText(value);
}

void
SetFormMultiLineValue(WndForm &form, const TCHAR *control_name,
                      const TCHAR *value)
{
  assert(control_name != NULL);
  assert(value != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  ctl->SetText(value, true);
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, bool value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  DataFieldBoolean &df = *(DataFieldBoolean *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_BOOLEAN);
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
  assert(df.GetType() == DataField::TYPE_ENUM);
  if (list[0].help != NULL)
    df.EnableItemHelp(true);

  df.AddChoices(list);
  df.Set(value);

  ctl->RefreshDisplay();
}

void
LoadFormProperty(WndForm &form, const TCHAR *control_name, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.Set(value);
  ctl->RefreshDisplay();
}

void
LoadOptionalFormProperty(WndForm &form, const TCHAR *control_name,
                         fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_REAL);
  df.Set(value);
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

  DataFieldString &df = *(DataFieldString *)ctl->GetDataField();
  assert(df.GetType() == DataField::TYPE_STRING);

  df.Set(value);
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
  assert(df.GetType() == DataField::TYPE_REAL);
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
  assert(df.GetType() == DataField::TYPE_BOOLEAN);
  return df.GetAsBoolean();
}

const TCHAR *
GetFormValueString(const WndForm &form, const TCHAR *control_name)
{
  assert(control_name != NULL);

  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  const DataFieldString &df = *(const DataFieldString *)control->GetDataField();
  assert(df.GetType() == DataField::TYPE_STRING);

  return df.GetAsString();
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
