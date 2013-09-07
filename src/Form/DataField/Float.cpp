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

#include "Float.hpp"
#include "ComboList.hpp"
#include "Asset.hpp"
#include "Util/NumberParser.hpp"

#include <stdlib.h>
#include <stdio.h>

static bool DataFieldKeyUp = false;

gcc_pure
static fixed
ParseString(const TCHAR *s)
{
  return fixed(ParseDouble(s));
}

int
DataFieldFloat::GetAsInteger() const
{
  return iround(mValue);
}

const TCHAR *
DataFieldFloat::GetAsString() const
{
  _stprintf(mOutBuf, edit_format, (double)mValue);
  return mOutBuf;
}

const TCHAR *
DataFieldFloat::GetAsDisplayString() const
{
  _stprintf(mOutBuf, display_format, (double)mValue, unit.c_str());
  return mOutBuf;
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
    Modified();
  }
}

void
DataFieldFloat::SetAsString(const TCHAR *Value)
{
  SetAsFloat(ParseString(Value));
}

void
DataFieldFloat::Inc()
{
  // no keypad, allow user to scroll small values
  if (mFine && mValue < fixed(0.95) && mStep >= fixed(0.5) &&
      mMin >= fixed(0))
    SetAsFloat(mValue + fixed(1) / 10);
  else
    SetAsFloat(fixed(mValue + mStep * SpeedUp(true)));
}

void
DataFieldFloat::Dec()
{
  // no keypad, allow user to scroll small values
  if (mFine && mValue <= fixed(1) && mStep >= fixed(0.5) &&
      mMin >= fixed(0))
    SetAsFloat(mValue - fixed(1) / 10);
  else
    SetAsFloat(fixed(mValue - mStep * SpeedUp(false)));
}

fixed
DataFieldFloat::SpeedUp(bool keyup)
{
  if (IsAltair())
    return fixed(1);

  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    last_step.Update();
    return fixed(1);
  }

  if (!last_step.Check(200)) {
    mSpeedup++;
    if (mSpeedup > 5) {
      last_step.UpdateWithOffset(350);
      return fixed(10);
    }
  } else
    mSpeedup = 0;

  last_step.Update();

  return fixed(1);
}

void
DataFieldFloat::SetFromCombo(int iDataFieldIndex, TCHAR *sValue)
{
  SetAsString(sValue);
}

void
DataFieldFloat::AppendComboValue(ComboList &combo_list, fixed value) const
{
  TCHAR a[edit_format.MAX_SIZE], b[display_format.MAX_SIZE];
  _stprintf(a, edit_format, (double)value);
  _stprintf(b, display_format, (double)value, unit.c_str());
  combo_list.Append(combo_list.size(), a, b);
}

ComboList
DataFieldFloat::CreateComboList(const TCHAR *reference_string) const
{
  const fixed reference = reference_string != nullptr
    ? ParseString(reference_string)
    : mValue;

  ComboList combo_list;
  const fixed epsilon = mStep / 1000;
  const fixed fine_step = mStep / 10;

  /* how many items before and after the current value? */
  unsigned surrounding_items = ComboList::MAX_SIZE / 2 - 2;
  if (mFine)
    surrounding_items -= 20;

  /* the value aligned to mStep */
  fixed corrected_value = int((reference - mMin) / mStep) * mStep + mMin;

  fixed first = corrected_value - surrounding_items * mStep;
  if (first > mMin + epsilon)
    /* there are values before "first" - give the user a choice */
    combo_list.Append(ComboList::Item::PREVIOUS_PAGE, _T("<<More Items>>"));
  else if (first < mMin - epsilon)
    first = int(mMin / mStep) * mStep;

  fixed last = std::min(first + surrounding_items * mStep * 2, mMax);

  bool found_current = false;
  fixed step = mStep;
  bool inFineSteps = false;
  for (fixed i = first; i <= last + epsilon; i += step) {

    // Skip over the items which fall below the beginning of the valid range.
    // e.g. first may be 0.0 for values with valid range 0.1 - 10.0 and step 1.0
    // rather than duplicate all the fine_step setup above simply ignore the few
    // values here. Needed for nice sequence e.g. 1.0 2.0 ... instead of 1.1 2.1 ...
    if (i < mMin - epsilon)
      continue;

    if (mFine) {
      // show up to 9 items above and below current value with extended precision
      if (i - epsilon > reference + mStep - fine_step) {
        if (inFineSteps) {
          inFineSteps = false;
          step = mStep;
          i = int((i + mStep - fine_step) / mStep) * mStep;
          if (i > mMax)
            i = mMax;
        }
      }
      else if (i + epsilon >= reference - mStep + fine_step) {
        if (!inFineSteps) {
          inFineSteps = true;
          step = fine_step;
          i = std::max(mMin, reference - mStep + fine_step);
        }
      }
    }

    if (!found_current && reference <= i + epsilon) {
      combo_list.ComboPopupItemSavedIndex = combo_list.size();

      if (reference < i - epsilon)
        /* the current value is not listed - insert it here */
        AppendComboValue(combo_list, reference);

      found_current = true;
    }

    AppendComboValue(combo_list, i);
  }

  if (reference > last + epsilon) {
    /* the current value out of range - append it here */
    last = reference;
    combo_list.ComboPopupItemSavedIndex = combo_list.size();
    AppendComboValue(combo_list, reference);
  }

  if (last < mMax - epsilon)
    /* there are values after "last" - give the user a choice */
    combo_list.Append(ComboList::Item::NEXT_PAGE, _T("<<More Items>>"));

  return combo_list;
}
