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
#include "Form/SubForm.hpp"
#include "Form/Edit.hpp"
#include "DataField/Base.hpp"
#include "DataField/Float.hpp"
#include "DataField/String.hpp"

#include <assert.h>

void
ShowFormControl(SubForm &form, const TCHAR *control_name, bool visible)
{
  Window *window = form.FindByName(control_name);
  assert(window != NULL);
  window->SetVisible(visible);
}

void
ShowOptionalFormControl(SubForm &form, const TCHAR *control_name,
                        bool visible)
{
  Window *window = form.FindByName(control_name);
  if (window != NULL)
    window->SetVisible(visible);
}

void
SetFormControlEnabled(SubForm &form, const TCHAR *control_name, bool enabled)
{
  Window *window = form.FindByName(control_name);
  assert(window != NULL);
  window->SetEnabled(enabled);
}

void
SetFormValue(SubForm &form, const TCHAR *control_name, const TCHAR *value)
{
  assert(control_name != NULL);
  assert(value != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  ctl->SetText(value);
}

void
LoadFormProperty(SubForm &form, const TCHAR *control_name, fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.Set(value);
  ctl->RefreshDisplay();
}

void
LoadOptionalFormProperty(SubForm &form, const TCHAR *control_name,
                         fixed value)
{
  assert(control_name != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  if (ctl == NULL)
    return;

  DataFieldFloat &df = *(DataFieldFloat *)ctl->GetDataField();
  assert(df.GetType() == DataField::Type::REAL);
  df.Set(value);
  ctl->RefreshDisplay();
}

void
LoadFormProperty(SubForm &form, const TCHAR *control_name,
                 const TCHAR *value)
{
  assert(control_name != NULL);
  assert(value != NULL);

  WndProperty *ctl = (WndProperty *)form.FindByName(control_name);
  assert(ctl != NULL);

  DataFieldString &df = *(DataFieldString *)ctl->GetDataField();
  assert(df.GetType() == DataField::Type::STRING);

  df.Set(value);
  ctl->RefreshDisplay();
}

int
GetFormValueInteger(const SubForm &form, const TCHAR *control_name)
{
  assert(control_name != NULL);

  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  return control->GetDataField()->GetAsInteger();
}

const TCHAR *
GetFormValueString(const SubForm &form, const TCHAR *control_name)
{
  assert(control_name != NULL);

  const WndProperty *control =
    (const WndProperty *)form.FindByName(control_name);
  assert(control != NULL);

  const DataFieldString &df = *(const DataFieldString *)control->GetDataField();
  assert(df.GetType() == DataField::Type::STRING);

  return df.GetAsString();
}
