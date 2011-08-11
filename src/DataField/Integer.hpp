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

#ifndef XCSOAR_DATA_FIELD_INTEGER_HPP
#define XCSOAR_DATA_FIELD_INTEGER_HPP

#include "DataField/Number.hpp"
#include "PeriodClock.hpp"

class DataFieldInteger : public NumberDataField
{
private:
  int mValue;
  int mMin;
  int mMax;
  int mStep;
  PeriodClock last_step;
  int mSpeedup;
  TCHAR mOutBuf[OUTBUFFERSIZE + 1];

protected:
  int SpeedUp(bool keyup);

public:
  DataFieldInteger(TCHAR *EditFormat, TCHAR *DisplayFormat, int Min, int Max,
                   int Default, int Step, DataAccessCallback_t OnDataAccess)
    :NumberDataField(EditFormat, DisplayFormat, OnDataAccess),
     mValue(Default), mMin(Min), mMax(Max), mStep(Step) {
    SupportCombo = true;
  }

  void Inc(void);
  void Dec(void);
  virtual ComboList *CreateComboList() const;
  virtual void SetFromCombo(int iDataFieldIndex, TCHAR *sValue);

  virtual int GetAsInteger(void) const;
  virtual const TCHAR *GetAsString(void) const;
  virtual const TCHAR *GetAsDisplayString(void) const;

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif

  void Set(int Value);
  int SetMin(int Value) { mMin = Value; return mMin; }
  int SetMax(int Value) { mMax = Value; return mMax; }

  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif

  virtual void SetAsInteger(int Value);
  virtual void SetAsString(const TCHAR *Value);

protected:
  void AppendComboValue(ComboList &combo_list, int value) const;
};

#endif
