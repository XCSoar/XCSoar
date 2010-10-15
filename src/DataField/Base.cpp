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

#include "DataField/Base.hpp"
#include "DataField/ComboList.hpp"

#include <math.h>
#include <algorithm>

using std::min;

enum { ComboPopupITEMMAX = 100 };

void
DataField::Special(void)
{
  (mOnDataAccess)(this, daSpecial);
}

void
DataField::Inc(void)
{
  (mOnDataAccess)(this, daInc);
}

void
DataField::Dec(void)
{
  (mOnDataAccess)(this, daDec);
}

bool
DataField::GetAsBoolean() const
{
  return false;
}

int
DataField::GetAsInteger() const
{
  return 0;
}

fixed
DataField::GetAsFixed() const
{
  return fixed_zero;
}

const TCHAR *
DataField::GetAsString() const
{
  return NULL;
}

const TCHAR *
DataField::GetAsDisplayString() const
{
  return GetAsString();
}

void
DataField::SetAsBoolean(bool Value)
{
  (void)Value;
}

void
DataField::SetAsInteger(int Value)
{
  (void)Value;
}

void
DataField::SetAsFloat(fixed Value)
{
  (void)Value;
}

void
DataField::SetAsString(const TCHAR *Value)
{
  (void)Value;
}

static void
__Dummy(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  (void)Sender;
  (void)Mode;
}

DataField::DataField(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                     DataAccessCallback_t OnDataAccess)
  :mOnDataAccess(OnDataAccess != NULL ? OnDataAccess : __Dummy),
   mUsageCounter(0), mDisableSpeedup(false), mDetachGUI(false)
{
  _tcscpy(mEditFormat, EditFormat);
  _tcscpy(mDisplayFormat, DisplayFormat);

  // blank units
  mUnits[0]= 0;
}

void
DataField::SetDisplayFormat(TCHAR *Value)
{
  _tcscpy(mDisplayFormat, Value);
}

void
DataField::CopyString(TCHAR * szbuffOut, bool bFormatted)
{
  int iLen = 0;
  if (!bFormatted) {
    if (GetAsString() != NULL) {
      // null leaves iLen=0
      iLen = _tcslen(GetAsString());
      _tcsncpy(szbuffOut, GetAsString(), min(iLen, ComboPopupITEMMAX - 1));
    }
  } else {
    if (GetAsDisplayString() != NULL) {
      iLen = _tcslen(GetAsDisplayString());
      _tcsncpy(szbuffOut, GetAsDisplayString(),
          min(iLen, ComboPopupITEMMAX - 1));
    }
  }
  szbuffOut[min(iLen, ComboPopupITEMMAX - 1)] = '\0';
}

ComboList *
DataField::CreateComboListStepping(void)
{
  // for DataFieldInteger and DataFieldFloat
  // builds ComboPopupItemList[] by calling CreateItem for each item in list
  // sets ComboPopupItemSavedIndex (global)
  // returns ComboPopupItemCount
  const fixed ComboListInitValue(-99999);
  const fixed ComboFloatPrec(0.0000001); //rounds float errors to this precision

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

    if (fNext == fCurrent) // we're at start of the list
      break;
    if (fNext == fLast) // don't repeat Yes/No/etc  (is this needed w/out Bool?)
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
    combo_list->Append(combo_list->size(), ComboList::Item::PREVIOUS_PAGE,
                       _T("<<More Items>>"), _T("<<More Items>>"));
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
          combo_list->Append(combo_list->size(), 0, PropertyValueSaved,
                             PropertyValueSavedFormatted);
      }
    }

    if (iSelectedIndex == -1 && fabs(fCurrent - fSavedValue) < ComboFloatPrec) {
      // selected item index
      iSelectedIndex = combo_list->size();
    }

    CopyString(sTemp, true); // can't call GetAsString & GetAsStringFormatted together (same output buffer)
    combo_list->Append(combo_list->size(), 0, GetAsString(), sTemp);

    Inc();
    fNext = GetAsFixed();

    if (fNext == fCurrent)
      // we're at start of the list
      break;

    if (fNext == fLast && combo_list->size() > 0)
      //we're at the end of the range
      break;

    fLast = fCurrent;
    fCurrent = fNext;
  }

  // if we stopped before hitting end of list create <<More>> value at end of list
  if (iListCount == ComboList::MAX_SIZE - 3) {
    // this data index item is checked on close of dialog
    combo_list->Append(combo_list->size(), ComboList::Item::NEXT_PAGE,
                       _T("<<More Items>>"), _T("<<More Items>>"));
  }

  SetDisableSpeedUp(false);
  SetDetachGUI(false); // disable dispaly of inc/dec/change values

  if (iSelectedIndex >= 0)
    SetAsFloat(fSavedValue);

  combo_list->ComboPopupItemSavedIndex = iSelectedIndex;

  return combo_list;
}
