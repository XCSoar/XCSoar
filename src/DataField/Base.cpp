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

#include "DataField/Base.hpp"

#include <math.h>
#include <algorithm>

using std::min;

enum { ComboPopupITEMMAX = 100 };

void
DataField::Special(void)
{
  if (mOnDataAccess != NULL)
    mOnDataAccess(this, daSpecial);
}

void
DataField::Inc(void)
{
  if (mOnDataAccess != NULL)
    mOnDataAccess(this, daInc);
}

void
DataField::Dec(void)
{
  if (mOnDataAccess != NULL)
    mOnDataAccess(this, daDec);
}

int
DataField::GetAsInteger() const
{
  return 0;
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
DataField::SetAsInteger(int Value)
{
  (void)Value;
}

void
DataField::SetAsString(const TCHAR *Value)
{
  (void)Value;
}

DataField::DataField(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                     DataAccessCallback_t OnDataAccess)
  :mOnDataAccess(OnDataAccess),
   mItemHelp(false), mUsageCounter(0), mDisableSpeedup(false), mDetachGUI(false)
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
