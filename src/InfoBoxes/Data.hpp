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

#ifndef XCSOAR_INFO_BOX_DATA_HPP
#define XCSOAR_INFO_BOX_DATA_HPP

#include "util/StaticString.hxx"
#include "Units/Unit.hpp"
#include "time/FloatDuration.hxx"

#include <chrono>
#include <cstdint>

#include <tchar.h>

class Angle;

struct InfoBoxData {
  static constexpr unsigned COLOR_COUNT = 6;

  /**
   * If non-zero, then custom painting is enabled via
   * InfoBoxContent::OnCustomPaint().  The integer value can be used
   * to detect changes.
   */
  uint64_t custom;

  unsigned content_serial;

  StaticString<32> title;
  StaticString<32> value;
  StaticString<32> comment;

  Unit value_unit;

  uint8_t title_color, value_color, comment_color;

  void Clear() noexcept;

  /**
   * Enable custom painting via InfoBoxContent::OnCustomPaint().
   *
   * @param _custom a non-zero value which should change each time a
   * different content will be drawn
   */
  void SetCustom(uint64_t _custom) noexcept {
    custom = _custom;
    value.clear();
  }

  uint64_t GetCustom() const noexcept {
    return custom;
  }

  /**
   * Resets value to --- and unassigns the unit
   */
  void SetValueInvalid() noexcept;

  /**
   * Clears comment
   */
  void SetCommentInvalid() noexcept {
    comment.clear();
  }

  /**
   * calls SetValueInvalid() then SetCommentInvalid()
   */
  void SetInvalid() noexcept;

  /**
   * Sets the InfoBox title to the given Value
   *
   * @param title New value of the InfoBox title
   */
  void SetTitle(const TCHAR *title) noexcept;

  const TCHAR *GetTitle() const {
    return title;
  };

  /**
   * Sets the InfoBox value to the given Value
   * @param Value New value of the InfoBox value
   */
  void SetValue(const TCHAR *value) noexcept;

  void SetValue(const TCHAR *format, double value) noexcept;

  /**
   * Sets the InfoBox value to the given angle.
   */
  void SetValue(Angle value, const TCHAR *suffix=_T("")) noexcept;

  void SetValueFromBearingDifference(Angle delta) noexcept;

  /**
   * Set the InfoBox value to the specified glide ratio.
   */
  void SetValueFromGlideRatio(double gr) noexcept;

  /**
   * Set the InfoBox value to the specified distance.
   */
  void SetValueFromDistance(double value) noexcept;

  /**
   * Set the InfoBox value to the specified altitude.
   */
  void SetValueFromAltitude(double value) noexcept;

  /**
   * Set the InfoBox value to the specified arrival altitude.
   */
  void SetValueFromArrival(double value) noexcept;

  /**
   * Set the InfoBox value to the specified horizontal speed.
   */
  void SetValueFromSpeed(double value, bool precision=true) noexcept;

  /**
   * Set the InfoBox value to the specified task speed.
   */
  void SetValueFromTaskSpeed(double value, bool precision=true) noexcept;

  /**
   * Set the InfoBox value to the specified percentage value.
   */
  void SetValueFromPercent(double value) noexcept;

  /**
   * Set the InfoBox value to the specified voltage value.
   */
  void SetValueFromVoltage(double value) noexcept;

  /**
   * Sets the InfoBox comment to the given Value
   * @param Value New value of the InfoBox comment
   */
  void SetComment(const TCHAR *comment) noexcept;

  /**
   * Sets the InfoBox comment to the given angle.
   */
  void SetComment(Angle comment, const TCHAR *suffix=_T("")) noexcept;

  void SetCommentFromDistance(double value) noexcept;

  void SetCommentFromBearingDifference(Angle delta) noexcept;

  /**
   * Set the InfoBox comment to the specified horizontal speed.
   */
  void SetCommentFromSpeed(double value, bool precision=true) noexcept;

  /**
   * Set the InfoBox comment to the specified task speed.
   */
  void SetCommentFromTaskSpeed(double value, bool precision=true) noexcept;

  /**
   * Set the InfoBox value to the specified altitude in the alternate
   * altitude unit.
   */
  void SetCommentFromAlternateAltitude(double value) noexcept;

  /**
   * Set the InfoBox comment value to the specified vertical speed.
   */
  void SetCommentFromVerticalSpeed(double value, bool include_sign=true) noexcept;

  /**
   * Set the InfoBox value to time HH:MM and SS
   */
  void SetValueFromTimeTwoLines(std::chrono::seconds dd) noexcept;

  void SetValueFromTimeTwoLines(FloatDuration dd) noexcept {
    SetValueFromTimeTwoLines(std::chrono::duration_cast<std::chrono::seconds>(dd));
  }

  /**
   * Set the InfoBox comment to the specified percentage value.
   */
  void SetCommentFromPercent(double value) noexcept;

  template<typename... Args>
  void FormatTitle(const TCHAR *fmt, Args&&... args) noexcept {
    title.Format(fmt, args...);
    title.CropIncompleteUTF8();
  }

  template<typename... Args>
  void FormatValue(const TCHAR *fmt, Args&&... args) noexcept {
    value.Format(fmt, args...);
  }

  template<typename... Args>
  void FormatComment(const TCHAR *fmt, Args&&... args) noexcept {
    comment.Format(fmt, args...);
    comment.CropIncompleteUTF8();
  }

  template<typename... Args>
  void UnsafeFormatValue(const TCHAR *fmt, Args&&... args) noexcept {
    value.UnsafeFormat(fmt, args...);
  }

  template<typename... Args>
  void UnsafeFormatComment(const TCHAR *fmt, Args&&... args) noexcept {
    comment.UnsafeFormat(fmt, args...);
  }

  /**
   * Sets the unit of the InfoBox value
   *
   * @param Value New unit of the InfoBox value
   */
  void SetValueUnit(Unit _value_unit) noexcept {
    value_unit = _value_unit;
  }

  /**
   * Sets the color of the InfoBox value to the given value
   * @param value New color of the InfoBox value
   */
  void SetValueColor(unsigned _color) noexcept {
    assert(_color < COLOR_COUNT);

    value_color = _color;
  }

  /**
   * Sets the color of the InfoBox comment to the given value
   * @param value New color of the InfoBox comment
   */
  void SetCommentColor(unsigned _color) noexcept {
    assert(_color < COLOR_COUNT);

    comment_color = _color;
  }

  /**
   * Sets the color of the InfoBox title to the given value
   * @param value New color of the InfoBox title
   */
  void SetTitleColor(unsigned _color) noexcept {
    assert(_color < COLOR_COUNT);

    title_color = _color;
  }

  void SetAllColors(unsigned color) noexcept;

  bool CompareCustom(const InfoBoxData &other) const noexcept {
    return content_serial == other.content_serial &&
      custom == other.custom;
  }

  bool CompareTitle(const InfoBoxData &other) const noexcept;
  bool CompareValue(const InfoBoxData &other) const noexcept;
  bool CompareComment(const InfoBoxData &other) const noexcept;
};

#endif
