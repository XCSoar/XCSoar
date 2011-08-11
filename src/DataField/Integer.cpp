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

#include "DataField/Integer.hpp"
#include "DataField/ComboList.hpp"
#include "Math/FastMath.h"
#include "Compatibility/string.h"
#include "Asset.hpp"

#include <stdlib.h>
#include <stdio.h>

static bool DataFieldKeyUp = false;

int
DataFieldInteger::GetAsInteger(void) const
{
  return mValue;
}

const TCHAR *
DataFieldInteger::GetAsString(void) const
{
  TCHAR *mOutBuf = const_cast<TCHAR *>(this->mOutBuf);
  _stprintf(mOutBuf, edit_format, mValue);
  return mOutBuf;
}

const TCHAR *
DataFieldInteger::GetAsDisplayString(void) const
{
  TCHAR *mOutBuf = const_cast<TCHAR *>(this->mOutBuf);
  _stprintf(mOutBuf, display_format, mValue, unit.c_str());
  return mOutBuf;
}

void
DataFieldInteger::Set(int Value)
{
  mValue = Value;
}

void
DataFieldInteger::SetAsInteger(int Value)
{
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (mValue != Value) {
    mValue = Value;
    if (!GetDetachGUI() && mOnDataAccess != NULL)
      (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldInteger::SetAsString(const TCHAR *Value)
{
  SetAsInteger(_ttoi(Value));
}

void
DataFieldInteger::Inc(void)
{
  SetAsInteger(mValue + mStep * SpeedUp(true));
}

void
DataFieldInteger::Dec(void)
{
  SetAsInteger(mValue - mStep * SpeedUp(false));
}

int
DataFieldInteger::SpeedUp(bool keyup)
{
  int res = 1;

  if (is_altair())
    return res;

  if (GetDisableSpeedUp() == true)
    return 1;

  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    last_step.update();
    return 1;
  }

  if (!last_step.check(200)) {
    mSpeedup++;
    if (mSpeedup > 5) {
      res = 10;
      last_step.update_offset(350);
      return (res);
    }
  } else
    mSpeedup = 0;

  last_step.update();

  return res;
}

void
DataFieldInteger::AppendComboValue(ComboList &combo_list, int value) const
{
  TCHAR a[edit_format.MAX_SIZE], b[display_format.MAX_SIZE];
  _stprintf(a, edit_format, value);
  _stprintf(b, display_format, value, unit.c_str());
  combo_list.Append(combo_list.size(), a, b);
}

ComboList *
DataFieldInteger::CreateComboList() const
{
  ComboList *combo_list = new ComboList();

  /* how many items before and after the current value? */
  unsigned surrounding_items = ComboList::MAX_SIZE / 2 - 2;

  /* the value aligned to mStep */
  int corrected_value = ((mValue - mMin) / mStep) * mStep;

  int first = corrected_value - (int)surrounding_items * mStep;
  if (first > mMin)
    /* there are values before "first" - give the user a choice */
    combo_list->Append(ComboList::Item::PREVIOUS_PAGE, _T("<<More Items>>"));
  else if (first < mMin)
    first = mMin;

  int last = std::min(first + (int)surrounding_items * mStep * 2, mMax);

  bool found_current = false;
  for (int i = first; i <= last; i += mStep) {
    if (!found_current && mValue <= i) {
      if (mValue < i)
        /* the current value is not listed - insert it here */
        AppendComboValue(*combo_list, mValue);

      combo_list->ComboPopupItemSavedIndex = combo_list->size();
      found_current = true;
    }

    AppendComboValue(*combo_list, i);
  }

  if (mValue > last) {
    /* the current value out of range - append it here */
    AppendComboValue(*combo_list, mValue);
    combo_list->ComboPopupItemSavedIndex = combo_list->size();
  }

  if (last < mMax)
    /* there are values after "last" - give the user a choice */
    combo_list->Append(ComboList::Item::NEXT_PAGE, _T("<<More Items>>"));

  return combo_list;
}

void
DataFieldInteger::SetFromCombo(int iDataFieldIndex, TCHAR *sValue)
{
  SetAsString(sValue);
}
