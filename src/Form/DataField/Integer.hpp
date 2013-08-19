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

#ifndef XCSOAR_DATA_FIELD_INTEGER_HPP
#define XCSOAR_DATA_FIELD_INTEGER_HPP

#include "Number.hpp"
#include "Time/PeriodClock.hpp"

class DataFieldInteger final : public NumberDataField
{
private:
  int value;
  int min;
  int max;
  int step;
  PeriodClock last_step;
  int speedup;

  mutable TCHAR output_buffer[OUTBUFFERSIZE + 1];

protected:
  int SpeedUp(bool keyup);

public:
  DataFieldInteger(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                   int Min, int Max,
                   int Default, int Step, DataAccessCallback OnDataAccess)
    :NumberDataField(Type::INTEGER, true, EditFormat, DisplayFormat, OnDataAccess),
     value(Default), min(Min), max(Max), step(Step) {}

  DataFieldInteger(const TCHAR *edit_format, const TCHAR *display_format,
                   int _min, int _max, int _value, int _step,
                   DataFieldListener *listener=nullptr)
    :NumberDataField(Type::REAL, true, edit_format, display_format, listener),
     value(_value), min(_min), max(_max), step(_step) {}

  void Set(int _value) {
    value = _value;
  }

  void SetMin(int _min) {
    min = _min;
  }

  void SetMax(int _max) {
    max = _max;
  }

  /* virtual methods from class DataField */
  virtual void Inc() override;
  virtual void Dec() override;
  virtual int GetAsInteger() const override;
  virtual const TCHAR *GetAsString() const override;
  virtual const TCHAR *GetAsDisplayString() const override;
  virtual void SetAsInteger(int value) override;
  virtual void SetAsString(const TCHAR *value) override;
  virtual ComboList CreateComboList(const TCHAR *reference) const override;
  virtual void SetFromCombo(int iDataFieldIndex, TCHAR *sValue) override;

protected:
  void AppendComboValue(ComboList &combo_list, int value) const;
};

#endif
