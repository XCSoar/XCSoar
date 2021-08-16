/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "time/PeriodClock.hpp"

class DataFieldFloat final : public NumberDataField {
  double mValue;
  double mMin;
  double mMax;
  double mStep;
  PeriodClock last_step;
  uint8_t mSpeedup;
  bool mFine;

  StaticString<8> unit;

  mutable TCHAR mOutBuf[OUTBUFFERSIZE+1];

protected:
  double SpeedUp(bool keyup) noexcept;

public:
  DataFieldFloat(const TCHAR *edit_format, const TCHAR *display_format,
                 double _min, double _max, double _value,
                 double _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :NumberDataField(Type::REAL, true, edit_format, display_format, listener),
     mValue(_value), mMin(_min), mMax(_max), mStep(_step),
     mSpeedup(0), mFine(_fine),
     unit(_T("")) {}

  void SetUnits(const TCHAR *text) noexcept {
    unit = text;
  }

  void SetMin(double v) noexcept {
    mMin = v;
  }

  void SetMax(double v) noexcept {
    mMax = v;
  }

  void SetStep(double v) noexcept {
    mStep = v;
  }

  double GetStep() const noexcept {
    return mStep;
  }

  double GetValue() const noexcept {
    return mValue;
  }

  void SetValue(double _value) noexcept {
    mValue = _value;
  }

  void ModifyValue(double Value) noexcept;

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  int GetAsInteger() const noexcept override;
  const TCHAR *GetAsString() const noexcept override;
  const TCHAR *GetAsDisplayString() const noexcept override;
  void SetAsInteger(int value) noexcept override;
  void SetAsString(const TCHAR *value) noexcept override;
  ComboList CreateComboList(const TCHAR *reference) const noexcept override;
  void SetFromCombo(int iDataFieldIndex, const TCHAR *sValue) noexcept override;

protected:
  void AppendComboValue(ComboList &combo_list, double value) const noexcept;
};

#endif
