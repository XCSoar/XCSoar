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

#ifndef XCSOAR_DATA_FIELD_FLOAT_HPP
#define XCSOAR_DATA_FIELD_FLOAT_HPP

#include "DataField/Number.hpp"
#include "PeriodClock.hpp"
#include "Math/fixed.hpp"

class DataFieldFloat : public NumberDataField
{
private:
  fixed mValue;
  fixed mMin;
  fixed mMax;
  fixed mStep;
  PeriodClock last_step;
  int mSpeedup;
  int mFine;

  StaticString<8> unit;

  mutable TCHAR mOutBuf[OUTBUFFERSIZE+1];

protected:
  fixed SpeedUp(bool keyup);


public:
  DataFieldFloat(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                 fixed Min, fixed Max, fixed Default,
                 fixed Step, int Fine, DataAccessCallback_t OnDataAccess)
    :NumberDataField(TYPE_REAL, EditFormat, DisplayFormat, OnDataAccess),
     mValue(Default), mMin(Min), mMax(Max), mStep(Step), mFine(Fine),
     unit(_T(""))
  {
    SupportCombo=true;
  }

  void SetUnits(const TCHAR *text) {
    unit = text;
  }

  void Inc(void);
  void Dec(void);
  virtual ComboList *CreateComboList() const;
  void SetFromCombo(int iDataFieldIndex, TCHAR *sValue);

  virtual int GetAsInteger(void) const;
  fixed GetAsFixed() const;
  virtual const TCHAR *GetAsString(void) const;
  virtual const TCHAR *GetAsDisplayString(void) const;

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(fixed Value);
  fixed SetMin(fixed Value);
  fixed SetMax(fixed Value);
  fixed SetStep(fixed Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  virtual void SetAsInteger(int Value);
  void SetAsFloat(fixed Value);
  virtual void SetAsString(const TCHAR *Value);

protected:
  ComboList *CreateComboListStepping();
};

#endif
