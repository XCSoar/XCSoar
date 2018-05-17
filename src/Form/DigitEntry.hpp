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

#ifndef XCSOAR_FORM_DIGIT_ENTRY_HPP
#define XCSOAR_FORM_DIGIT_ENTRY_HPP

#include "Screen/PaintWindow.hpp"
#include "Renderer/ButtonRenderer.hpp"

#include <assert.h>
#include <stdint.h>

enum class CoordinateFormat : uint8_t;
class RoughTime;
class Angle;
class ContainerWindow;
class ActionListener;
struct DialogLook;

/**
 * A control that allows entering numbers or other data types digit by
 * digit.  It aims to be usable for both touch screens and knob-only.
 */
class DigitEntry : public PaintWindow {
  static constexpr unsigned MAX_LENGTH = 16;

  struct Column {
    enum class Type : uint8_t {
      DIGIT,
      DIGIT6,
      HOUR, // i.e. DIGIT24
      DIGIT19,
      DIGIT36,
      SIGN,
      DECIMAL_POINT,
      COLON,
      NORTH_SOUTH,
      EAST_WEST,
      UNIT,
      DEGREES,
      APOSTROPHE,
      QUOTE,
    };

    Type type;

    uint8_t value;

    unsigned left, right;

    constexpr bool IsSign() const {
      return type == Type::SIGN ||
        type == Type::NORTH_SOUTH || type == Type::EAST_WEST;
    }

    constexpr bool IsNegative() const {
      return value != 0;
    }

    constexpr bool IsNumber() const {
      return type == Type::DIGIT ||
        type == Type::DIGIT6 ||
        type == Type::DIGIT19 ||
        type == Type::DIGIT36 ||
        type == Type::HOUR;
    }

    constexpr unsigned GetMaxNumber() const {
      return type == Type::DIGIT6 ? 5 :
             type == Type::HOUR ? 23 :
             type == Type::DIGIT19 ? 18 :
             type == Type::DIGIT36 ? 35 : 9;
    }

    constexpr bool IsEditable() const {
      return IsNumber() || IsSign();
    }

    void SetNegative(bool is_negative) {
      assert(IsSign());

      value = is_negative;
    }

    /**
     * Returns the number characters that this column can have.
     * Used for calculating the pixel-based width of the column.
     */
    constexpr unsigned GetWidth() const {
      return type == Type::UNIT ? 4 :
             type == Type::HOUR || type == Type::DIGIT19 || type == Type::DIGIT36 ? 2 : 1;
    }
  };

  const DialogLook &look;

  ButtonFrameRenderer button_renderer;

  ActionListener *action_listener;
  int action_id;

  /**
   * Total number of columns.
   */
  unsigned length;

  Column columns[MAX_LENGTH];

  bool valid;

  unsigned top, bottom, max_width;

  /**
   * The current digit.
   */
  unsigned cursor;

public:
  DigitEntry(const DialogLook &_look);
  virtual ~DigitEntry();

protected:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              const WindowStyle style,
              unsigned length);

public:
  void CreateSigned(ContainerWindow &parent, const PixelRect &rc,
                    const WindowStyle style,
                    unsigned ndigits, unsigned precision);

  void CreateUnsigned(ContainerWindow &parent, const PixelRect &rc,
                      const WindowStyle style,
                      unsigned ndigits, unsigned precision);

  void CreateTime(ContainerWindow &parent, const PixelRect &rc,
                  const WindowStyle style);

  void CreateAngle(ContainerWindow &parent, const PixelRect &rc,
                   const WindowStyle style);

  void CreateLatitude(ContainerWindow &parent, const PixelRect &rc,
                      const WindowStyle style, CoordinateFormat format);

  void CreateLongitude(ContainerWindow &parent, const PixelRect &rc,
                       const WindowStyle style, CoordinateFormat format);

  void CalculateLayout();

  gcc_pure
  PixelSize GetRecommendedSize() const {
    return PixelSize(columns[length - 1].right, bottom + top);
  }

  /**
   * Sets a listener that will be notified when the user "activates"
   * the control (for example by pressing the "enter" key).
   */
  void SetActionListener(ActionListener &listener, int id) {
    action_listener = &listener;
    action_id = id;
  }

  void SetCursor(unsigned cursor);

  void SetInvalid();

  void SetValue(int value);
  void SetValue(unsigned value);
  void SetValue(double value);
  void SetValue(RoughTime value);
  void SetValue(Angle value);

  gcc_pure
  int GetIntegerValue() const;

  gcc_pure
  unsigned GetUnsignedValue() const;

  gcc_pure
  double GetDoubleValue() const;

  gcc_pure
  RoughTime GetTimeValue() const;

  gcc_pure
  Angle GetAngleValue() const;

  void SetLatitude(Angle value, CoordinateFormat format);
  void SetLongitude(Angle value, CoordinateFormat format);

  gcc_pure
  Angle GetGeoAngle(CoordinateFormat format) const;

  gcc_pure
  Angle GetLatitude(CoordinateFormat format) const;

  gcc_pure
  Angle GetLongitude(CoordinateFormat format) const;

protected:
  gcc_pure
  bool IsSigned() const {
    return columns[0].IsSign();
  }

  gcc_pure
  bool IsNegative() const {
    return columns[0].IsSign() && columns[0].IsNegative();
  }

  /**
   * Find the first column with that contains a decimal point.
   */
  gcc_pure
  int FindDecimalPoint() const;

  /**
   * Find the next column to the left (including i) that is numerical.
   */
  gcc_pure
  int FindNumberLeft(int i) const;

  /**
   * Find the next column to the left (including i) that is editable.
   */
  gcc_pure
  int FindEditableLeft(int i) const;

  /**
   * Find the next column to the right (including i) that is editable.
   */
  gcc_pure
  int FindEditableRight(unsigned i) const;

  gcc_pure
  unsigned GetPositiveInteger() const;

  gcc_pure
  double GetPositiveFractional() const;

  void IncrementColumn(unsigned i);
  void DecrementColumn(unsigned i);

  gcc_pure
  int FindColumnAt(unsigned x) const;

private:
  void SetDigits(double degrees, CoordinateFormat format, bool isLatitude);

protected:
  void OnSetFocus() override;
  void OnKillFocus() override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;
  void OnPaint(Canvas &canvas) override;
};

#endif
