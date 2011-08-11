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

#include "DataField/Boolean.hpp"
#include "DataField/ComboList.hpp"

ComboList *
DataFieldBoolean::CreateComboList() const
{
  ComboList *combo_list = new ComboList();
  combo_list->Append(false, false_text);
  combo_list->Append(true, true_text);

  combo_list->ComboPopupItemSavedIndex = GetAsInteger();
  return combo_list;
}

bool
DataFieldBoolean::GetAsBoolean(void) const
{
  return mValue;
}

int
DataFieldBoolean::GetAsInteger(void) const
{
  if (mValue)
    return 1;
  else
    return 0;
}

const TCHAR *
DataFieldBoolean::GetAsString(void) const
{
  return mValue ? true_text : false_text;
}

void
DataFieldBoolean::Set(bool Value)
{
  mValue = Value;
}

void
DataFieldBoolean::SetAsBoolean(bool Value)
{
  if (mValue != Value) {
    mValue = Value;
    if (!GetDetachGUI() && mOnDataAccess != NULL)
      (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldBoolean::SetAsInteger(int Value)
{
  if (GetAsInteger() != Value) {
    SetAsBoolean(!(Value == 0));
  }
}

void
DataFieldBoolean::SetAsString(const TCHAR *Value)
{
  const TCHAR *res = GetAsString();
  if (_tcscmp(res, Value) != 0) {
    SetAsBoolean(true_text.equals(Value));
  }
}

void
DataFieldBoolean::Inc(void)
{
  SetAsBoolean(true);
}

void
DataFieldBoolean::Dec(void)
{
  SetAsBoolean(false);
}
