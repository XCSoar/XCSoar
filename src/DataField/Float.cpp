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

#include "DataField/Float.hpp"
#include "DataField/ComboList.hpp"
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

fixed
DataFieldFloat::SetStep(fixed Value)
{
  fixed res = mStep;
  mStep = Value;
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
    if (!GetDetachGUI() && mOnDataAccess != NULL)
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

enum { ComboPopupITEMMAX = 100 };

ComboList *
DataFieldFloat::CreateComboListStepping()
{
  // for DataFieldInteger and DataFieldFloat
  // builds ComboPopupItemList[] by calling CreateItem for each item in list
  // sets ComboPopupItemSavedIndex (global)
  // returns ComboPopupItemCount
  const fixed ComboListInitValue(-99999);
  const fixed ComboFloatPrec(0.0001); //rounds float errors to this precision

  fixed fNext = ComboListInitValue;
  fixed fCurrent = ComboListInitValue;
  fixed fLast = ComboListInitValue;
  TCHAR sTemp[ComboPopupITEMMAX];

  int iListCount = 0;
  int iSelectedIndex = -1;
  int iStepDirection = 1; // for integer & float step may be negative
  fixed fBeforeDec = fixed_zero, fAfterDec = fixed_zero, fSavedValue = fixed_zero;

  fNext = ComboListInitValue;
  fCurrent = ComboListInitValue;
  fLast = ComboListInitValue;

  SetDisableSpeedUp(true);
  SetDetachGUI(true); // disable display of inc/dec/change values

  // get step direction for int & float so we can detect if we skipped the value while iterating later
  TCHAR PropertyValueSaved[ComboPopupITEMMAX];
  TCHAR PropertyValueSavedFormatted[ComboPopupITEMMAX];
  CopyString(PropertyValueSaved, false);
  CopyString(PropertyValueSavedFormatted, true);

  fSavedValue = GetAsFixed();
  Inc();
  fBeforeDec = GetAsFixed();
  Dec();
  fAfterDec = GetAsFixed();

  if (fAfterDec < fBeforeDec)
    iStepDirection = 1;
  else
    iStepDirection = -1;

  // reset datafield to top of list (or for large floats, away from selected
  // item so it will be in the middle)
  for (iListCount = 0; iListCount < ComboList::MAX_SIZE / 2; iListCount++) {
    // for floats, go half way down only
    Dec();
    fNext = GetAsFixed();

    if (fabs(fNext - fCurrent) < ComboFloatPrec) // we're at start of the list
      break;
    if (fabs(fNext - fLast) < ComboFloatPrec) // don't repeat Yes/No/etc  (is this needed w/out Bool?)
      break;

    fLast = fCurrent;
    fCurrent = fNext;
  }

  fNext = ComboListInitValue;
  fCurrent = ComboListInitValue;
  fLast = ComboListInitValue;

  fCurrent = GetAsFixed();

  ComboList *combo_list = new ComboList();

  // if we stopped before hitting start of list create <<Less>> value at top of list
  if (iListCount == ComboList::MAX_SIZE / 2) {
    // this data index item is checked on close of dialog
    combo_list->Append(ComboList::Item::PREVIOUS_PAGE, _T("<<More Items>>"));
  }

  // now we're at the beginning of the list, so load forward until end
  for (iListCount = 0; iListCount < ComboList::MAX_SIZE - 3; iListCount++) {
    // stop at LISTMAX-3 b/c it may make an additional item if it's "off step", and
    // potentially two more items for <<More>> and << Less>>

    // test if we've stepped over the selected value which was not a multiple of the "step"
    if (iSelectedIndex == -1) {
      // not found yet
      if (iStepDirection * GetAsFixed() >
          (fSavedValue + ComboFloatPrec * iStepDirection)) {
        // step was too large, we skipped the selected value, so add it now
        iSelectedIndex =
          combo_list->Append(0, PropertyValueSaved,
                             PropertyValueSavedFormatted);
      }
    }

    if (iSelectedIndex == -1 && fabs(fCurrent - fSavedValue) < ComboFloatPrec) {
      // selected item index
      iSelectedIndex = combo_list->size();
    }

    CopyString(sTemp, true); // can't call GetAsString & GetAsStringFormatted together (same output buffer)
    combo_list->Append(0, GetAsString(), sTemp);

    Inc();
    fNext = GetAsFixed();

    if (fabs(fNext - fCurrent) < ComboFloatPrec)
      // we're at start of the list
      break;

    if ((fabs(fNext - fLast) < ComboFloatPrec) && combo_list->size() > 0)
      //we're at the end of the range
      break;

    fLast = fCurrent;
    fCurrent = fNext;
  }

  // if we stopped before hitting end of list create <<More>> value at end of list
  if (iListCount == ComboList::MAX_SIZE - 3) {
    // this data index item is checked on close of dialog
    combo_list->Append(ComboList::Item::NEXT_PAGE, _T("<<More Items>>"));
  }

  SetDisableSpeedUp(false);
  SetDetachGUI(false); // disable dispaly of inc/dec/change values

  if (iSelectedIndex >= 0)
    SetAsFloat(fSavedValue);

  combo_list->ComboPopupItemSavedIndex = iSelectedIndex;

  return combo_list;
}
