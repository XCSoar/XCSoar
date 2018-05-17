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

#ifndef XCSOAR_INFO_BOX_DATA_HPP
#define XCSOAR_INFO_BOX_DATA_HPP

#include "Util/StaticString.hxx"
#include "Units/Unit.hpp"

#include <tchar.h>

class Angle;

struct InfoBoxData {
  static constexpr unsigned COLOR_COUNT = 6;

  StaticString<32> title;
  StaticString<32> value;
  StaticString<32> comment;

  Unit value_unit;

  uint8_t title_color, value_color, comment_color;

  void Clear();

  /**
   * Enable custom painting via InfoBoxContent::OnCustomPaint().
   */
  void SetCustom() {
    /* 0xff is a "magic" value that indicates custom painting*/
    value_color = 0xff;
    value.clear();
  }

  bool GetCustom() const {
    return value_color == 0xff;
  }

  /**
   * Resets value to --- and unassigns the unit
   */
  void SetValueInvalid();

  /**
   * Clears comment
   */
  void SetCommentInvalid() {
    comment.clear();
  }

  /**
   * calls SetValueInvalid() then SetCommentInvalid()
   */
  void SetInvalid();

  /**
   * Sets the InfoBox title to the given Value
   *
   * @param title New value of the InfoBox title
   */
  void SetTitle(const TCHAR *title);

  const TCHAR *GetTitle() const {
    return title;
  };

  /**
   * Sets the InfoBox value to the given Value
   * @param Value New value of the InfoBox value
   */
  void SetValue(const TCHAR *value);

  void SetValue(const TCHAR *format, double value);

  /**
   * Sets the InfoBox value to the given angle.
   */
  void SetValue(Angle value, const TCHAR *suffix=_T(""));

  void SetValueFromBearingDifference(Angle delta);

  /**
   * Set the InfoBox value to the specified glide ratio.
   */
  void SetValueFromGlideRatio(double gr);

  /**
   * Set the InfoBox value to the specified distance.
   */
  void SetValueFromDistance(double value);

  /**
   * Set the InfoBox value to the specified altitude.
   */
  void SetValueFromAltitude(double value);

  /**
   * Set the InfoBox value to the specified arrival altitude.
   */
  void SetValueFromArrival(double value);

  /**
   * Set the InfoBox value to the specified horizontal speed.
   */
  void SetValueFromSpeed(double value, bool precision=true);

  /**
   * Set the InfoBox value to the specified task speed.
   */
  void SetValueFromTaskSpeed(double value, bool precision=true);

  /**
   * Set the InfoBox value to the specified percentage value.
   */
  void SetValueFromPercent(double value);

  /**
   * Set the InfoBox value to the specified voltage value.
   */
  void SetValueFromVoltage(double value);

  /**
   * Sets the InfoBox comment to the given Value
   * @param Value New value of the InfoBox comment
   */
  void SetComment(const TCHAR *comment);

  /**
   * Sets the InfoBox comment to the given angle.
   */
  void SetComment(Angle comment, const TCHAR *suffix=_T(""));

  void SetCommentFromDistance(double value);

  void SetCommentFromBearingDifference(Angle delta);

  /**
   * Set the InfoBox comment to the specified horizontal speed.
   */
  void SetCommentFromSpeed(double value, bool precision=true);

  /**
   * Set the InfoBox comment to the specified task speed.
   */
  void SetCommentFromTaskSpeed(double value, bool precision=true);

  /**
   * Set the InfoBox value to the specified altitude in the alternate
   * altitude unit.
   */
  void SetCommentFromAlternateAltitude(double value);

  /**
   * Set the InfoBox comment value to the specified vertical speed.
   */
  void SetCommentFromVerticalSpeed(double value, bool include_sign=true);

  /**
   * Set the InfoBox value to time HH:MM and SS
   */
  void SetValueFromTimeTwoLines(int dd);

  /**
   * Set the InfoBox comment to the specified percentage value.
   */
  void SetCommentFromPercent(double value);

  template<typename... Args>
  void FormatTitle(const TCHAR *fmt, Args&&... args) {
    title.Format(fmt, args...);
    title.CropIncompleteUTF8();
  }

  template<typename... Args>
  void FormatValue(const TCHAR *fmt, Args&&... args) {
    value.Format(fmt, args...);
  }

  template<typename... Args>
  void FormatComment(const TCHAR *fmt, Args&&... args) {
    comment.Format(fmt, args...);
    comment.CropIncompleteUTF8();
  }

  template<typename... Args>
  void UnsafeFormatValue(const TCHAR *fmt, Args&&... args) {
    value.UnsafeFormat(fmt, args...);
  }

  template<typename... Args>
  void UnsafeFormatComment(const TCHAR *fmt, Args&&... args) {
    comment.UnsafeFormat(fmt, args...);
  }

  /**
   * Sets the unit of the InfoBox value
   *
   * @param Value New unit of the InfoBox value
   */
  void SetValueUnit(Unit _value_unit) {
    value_unit = _value_unit;
  }

  /**
   * Sets the color of the InfoBox value to the given value
   * @param value New color of the InfoBox value
   */
  void SetValueColor(unsigned _color) {
    assert(_color < COLOR_COUNT);

    value_color = _color;
  }

  /**
   * Sets the color of the InfoBox comment to the given value
   * @param value New color of the InfoBox comment
   */
  void SetCommentColor(unsigned _color) {
    assert(_color < COLOR_COUNT);

    comment_color = _color;
  }

  /**
   * Sets the color of the InfoBox title to the given value
   * @param value New color of the InfoBox title
   */
  void SetTitleColor(unsigned _color) {
    assert(_color < COLOR_COUNT);

    title_color = _color;
  }

  void SetAllColors(unsigned color);

  bool CompareTitle(const InfoBoxData &other) const;
  bool CompareValue(const InfoBoxData &other) const;
  bool CompareComment(const InfoBoxData &other) const;
};

#endif
