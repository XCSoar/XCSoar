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

#ifndef XCSOAR_DATA_FIELD_FLOAT_HPP
#define XCSOAR_DATA_FIELD_FLOAT_HPP

#include "Number.hpp"
#include "Time/PeriodClock.hpp"
#include "Math/fixed.hpp"

class DataFieldFloat : public NumberDataField
{
private:
  fixed mValue;
  fixed mMin;
  fixed mMax;
  fixed mStep;
  PeriodClock last_step;
  uint8_t mSpeedup;
  bool mFine;

  StaticString<8> unit;

  mutable TCHAR mOutBuf[OUTBUFFERSIZE+1];

protected:
  fixed SpeedUp(bool keyup);


public:
  DataFieldFloat(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                 fixed Min, fixed Max, fixed Default,
                 fixed Step, bool Fine, DataAccessCallback OnDataAccess)
    :NumberDataField(Type::REAL, true, EditFormat, DisplayFormat, OnDataAccess),
     mValue(Default), mMin(Min), mMax(Max), mStep(Step),
     mSpeedup(0), mFine(Fine),
     unit(_T("")) {}

  void SetUnits(const TCHAR *text) {
    unit = text;
  }

  void Inc();
  void Dec();
  virtual ComboList *CreateComboList() const;
  void SetFromCombo(int iDataFieldIndex, TCHAR *sValue);

  virtual int GetAsInteger() const;
  virtual const TCHAR *GetAsString() const;
  virtual const TCHAR *GetAsDisplayString() const;

  void Set(fixed _value) {
    mValue = _value;
  }

  fixed GetAsFixed() const {
    return mValue;
  }

  void SetMin(fixed v) {
    mMin = v;
  }

  void SetMax(fixed v) {
    mMax = v;
  }

  void SetStep(fixed v) {
    mStep = v;
  }

  fixed GetStep() const {
    return mStep;
  }

  virtual void SetAsInteger(int Value);
  void SetAsFloat(fixed Value);
  virtual void SetAsString(const TCHAR *Value);

protected:
  void AppendComboValue(ComboList &combo_list, fixed value) const;
};

#endif
