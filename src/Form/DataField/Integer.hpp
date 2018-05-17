/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
  DataFieldInteger(const TCHAR *edit_format, const TCHAR *display_format,
                   int _min, int _max, int _value, int _step,
                   DataFieldListener *listener=nullptr)
    :NumberDataField(Type::INTEGER, true, edit_format, display_format, listener),
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
  void Inc() override;
  void Dec() override;
  int GetAsInteger() const override;
  const TCHAR *GetAsString() const override;
  const TCHAR *GetAsDisplayString() const override;
  void SetAsInteger(int value) override;
  void SetAsString(const TCHAR *value) override;
  ComboList CreateComboList(const TCHAR *reference) const override;
  void SetFromCombo(int iDataFieldIndex, const TCHAR *sValue) override;

protected:
  void AppendComboValue(ComboList &combo_list, int value) const;
};

#endif
