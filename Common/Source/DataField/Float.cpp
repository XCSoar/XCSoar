/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include <stdlib.h>
#include <stdio.h>

static bool DataFieldKeyUp = false;

bool DataFieldFloat::GetAsBoolean(void){
  return(mValue != 0.0);
}

int DataFieldFloat::GetAsInteger(void){
  return iround(mValue);
}

double DataFieldFloat::GetAsFloat(void){
  return(mValue);
}

TCHAR *DataFieldFloat::GetAsString(void){
  _stprintf(mOutBuf, mEditFormat, mValue);
  return(mOutBuf);
}

TCHAR *DataFieldFloat::GetAsDisplayString(void){
  _stprintf(mOutBuf, mDisplayFormat, mValue, mUnits);
  return(mOutBuf);
}

void DataFieldFloat::Set(double Value){
  mValue = Value;
}

double DataFieldFloat::SetMin(double Value){
  double res = mMin;
  mMin = Value;
  return(res);
};
double DataFieldFloat::SetMax(double Value){
  double res = mMax;
  mMax = Value;
  return(res);
};

bool DataFieldFloat::SetAsBoolean(bool Value){
  bool res = GetAsBoolean();
  if (res != Value){
    if (Value)
      SetAsFloat(1.0);
    else
      SetAsFloat(0.0);
  }
  return(res);
}

int DataFieldFloat::SetAsInteger(int Value){
  int res = GetAsInteger();
  SetAsFloat(Value);
  return(res);
}

double DataFieldFloat::SetAsFloat(double Value){
  double res = mValue;
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (res != Value){
    mValue = Value;
    if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
  }
  return(res);
}

TCHAR *DataFieldFloat::SetAsString(const TCHAR *Value){
  TCHAR *res = GetAsString();
  SetAsFloat(_tcstod(Value, NULL));
  return(res);
}

void DataFieldFloat::Inc(void){
  // no keypad, allow user to scroll small values
  if(mFine && (mValue < 0.95) && (mStep>=0.5) && (mMin>=0.0))
    {
      SetAsFloat(mValue + 0.1);
    }
  else
    SetAsFloat(mValue + mStep*SpeedUp(true));
}

void DataFieldFloat::Dec(void){
  // no keypad, allow user to scroll small values
  if(mFine && (mValue <= 1.0) && (mStep>=0.5) && (mMin>=0.0))
    {
      SetAsFloat(mValue - 0.1);
    }
  else
    SetAsFloat(mValue - mStep*SpeedUp(false));
}

double DataFieldFloat::SpeedUp(bool keyup){
  double res=1.0;

#ifdef GNAV
  return res;
#endif

  if (GetDisableSpeedUp() == true)
    return 1;

  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    mTmLastStep = GetTickCount();
    return 1.0;
  }

  if ((long)(GetTickCount()-mTmLastStep) < 200){
    mSpeedup++;

    if (mSpeedup > 5){
      res = 10;

      mTmLastStep = GetTickCount()+350;
      return(res);

    }
  } else
    mSpeedup = 0;

  mTmLastStep = GetTickCount();

  return(res);
}

int DataFieldFloat::SetFromCombo(int iDataFieldIndex, TCHAR *sValue) {
  SetAsString(sValue);
  return 0;
}

int DataFieldFloat::CreateComboList(void) {
  return CreateComboListStepping();
}
