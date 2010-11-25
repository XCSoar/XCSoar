/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "DataField/Float.hpp"
#include "Math/FastMath.h"
#include "Asset.hpp"

#include <stdlib.h>
#include <stdio.h>

static bool DataFieldKeyUp = false;

int
DataFieldFloat::GetAsInteger() const
{
  return iround(mValue);
}

fixed
DataFieldFloat::GetAsFixed() const
{
  return mValue;
}

const TCHAR *
DataFieldFloat::GetAsString() const
{
  _stprintf(mOutBuf, mEditFormat, (double)mValue);
  return mOutBuf;
}

const TCHAR *
DataFieldFloat::GetAsDisplayString() const
{
  _stprintf(mOutBuf, mDisplayFormat, (double)mValue, mUnits);
  return mOutBuf;
}

void
DataFieldFloat::Set(fixed Value)
{
  mValue = Value;
}

fixed
DataFieldFloat::SetMin(fixed Value)
{
  fixed res = mMin;
  mMin = Value;
  return res;
}

fixed
DataFieldFloat::SetMax(fixed Value)
{
  fixed res = mMax;
  mMax = Value;
  return res;
}

void
DataFieldFloat::SetAsInteger(int Value)
{
  SetAsFloat(fixed(Value));
}

void
DataFieldFloat::SetAsFloat(fixed Value)
{
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (mValue != Value) {
    mValue = Value;
    if (!GetDetachGUI())
      (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldFloat::SetAsString(const TCHAR *Value)
{
  SetAsFloat(fixed(_tcstod(Value, NULL)));
}

void
DataFieldFloat::Inc(void)
{
  // no keypad, allow user to scroll small values
  if (mFine && mValue < fixed(0.95) && mStep >= fixed_half &&
      mMin >= fixed_zero)
    SetAsFloat(mValue + fixed_one / 10);
  else
    SetAsFloat(fixed(mValue + mStep * SpeedUp(true)));
}

void
DataFieldFloat::Dec(void)
{
  // no keypad, allow user to scroll small values
  if (mFine && mValue <= fixed_one && mStep >= fixed_half &&
      mMin >= fixed_zero)
    SetAsFloat(mValue - fixed_one / 10);
  else
    SetAsFloat(fixed(mValue - mStep * SpeedUp(false)));
}

fixed
DataFieldFloat::SpeedUp(bool keyup)
{
  if (is_altair())
    return fixed_one;

  if (GetDisableSpeedUp() == true)
    return fixed_one;

  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    last_step.update();
    return fixed_one;
  }

  if (!last_step.check(200)) {
    mSpeedup++;
    if (mSpeedup > 5) {
      last_step.update_offset(350);
      return fixed_ten;
    }
  } else
    mSpeedup = 0;

  last_step.update();

  return fixed_one;
}

void
DataFieldFloat::SetFromCombo(int iDataFieldIndex, TCHAR *sValue)
{
  SetAsString(sValue);
}

ComboList *
DataFieldFloat::CreateComboList() const
{
  DataFieldFloat clone(*this);
  return clone.CreateComboListStepping();
}
