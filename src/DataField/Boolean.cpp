/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
  combo_list->Append(0, 0, mTextFalse, mTextFalse);
  combo_list->Append(1, 1, mTextTrue, mTextTrue);

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

fixed
DataFieldBoolean::GetAsFixed() const
{
  return mValue ? fixed_one : fixed_zero;
}

const TCHAR *
DataFieldBoolean::GetAsString(void) const
{
  if (mValue)
    return mTextTrue;
  else
    return mTextFalse;
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
    if (!GetDetachGUI())
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
DataFieldBoolean::SetAsFloat(fixed Value)
{
  if (GetAsFixed() != Value) {
    SetAsBoolean(Value != fixed_zero);
  }
}

void
DataFieldBoolean::SetAsString(const TCHAR *Value)
{
  const TCHAR *res = GetAsString();
  if (_tcscmp(res, Value) != 0) {
    SetAsBoolean(_tcscmp(Value, mTextTrue) == 0);
  }
}

void
DataFieldBoolean::Inc(void)
{
  SetAsBoolean(!GetAsBoolean());
}

void
DataFieldBoolean::Dec(void)
{
  SetAsBoolean(!GetAsBoolean());
}
