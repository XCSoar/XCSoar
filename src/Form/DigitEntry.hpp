// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Renderer/ButtonRenderer.hpp"

#include <cassert>
#include <cstdint>
#include <functional>

enum class CoordinateFormat : uint8_t;
class RoughTime;
class Angle;
struct BrokenDate;
class ContainerWindow;
struct DialogLook;
class RadioFrequency;

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
      DAY,
      MONTH,
      YEAR,
      RADIO_FREQURENCY_MHZ_OFFSET, // Actual value minus 118
      RADIO_FREQURENCY_KHZ_SCALED, // Last 2 digits divided by 5
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
        type == Type::HOUR ||
        type == Type::DAY ||
        type == Type::MONTH ||
        type == Type::YEAR ||
        type == Type::RADIO_FREQURENCY_MHZ_OFFSET ||
        type == Type::RADIO_FREQURENCY_KHZ_SCALED;
    }

    constexpr bool NoOverflow() const {
      return type == Type::DAY ||
        type == Type::MONTH ||
        type == Type::YEAR;

    }

    constexpr unsigned GetMaxNumber() const {
      switch (type) {
      case Type::HOUR:
        return 23;

      case Type::DAY:
        return 30;

      case Type::MONTH:
        return 11;

      case Type::YEAR:
        return 199;

      case Type::DIGIT6:
        return 5;

      case Type::DIGIT19:
        return 18;

      case Type::RADIO_FREQURENCY_MHZ_OFFSET:
        return 18;

      case Type::RADIO_FREQURENCY_KHZ_SCALED:
        return 19;

      case Type::DIGIT36:
        return 35;   

      default:
        return 9;
      }
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
      switch (type) {
      case Type::DAY:
        return 2;

      case Type::MONTH:
        return 3;

      case Type::YEAR:
        return 5;

      case Type::UNIT:
        return 4;

      case Type::HOUR:
      case Type::DIGIT19:
      case Type::DIGIT36:
        return 2;

      default:
        return 1;
      }
    }
  };

  const DialogLook &look;

  ButtonFrameRenderer button_renderer;

  std::function<void()> callback;

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

  void CreateDate(ContainerWindow &parent, const PixelRect &rc,
                  const WindowStyle style);

  void CreateAngle(ContainerWindow &parent, const PixelRect &rc,
                   const WindowStyle style);

  void CreateLatitude(ContainerWindow &parent, const PixelRect &rc,
                      const WindowStyle style, CoordinateFormat format);

  void CreateLongitude(ContainerWindow &parent, const PixelRect &rc,
                       const WindowStyle style, CoordinateFormat format);

  void CreateRadioFrequency(ContainerWindow &parent, const PixelRect &rc,
                             const WindowStyle style);

  void CalculateLayout();

  [[gnu::pure]]
  PixelSize GetRecommendedSize() const {
    return PixelSize(columns[length - 1].right, bottom + top);
  }

  /**
   * Sets a listener that will be notified when the user "activates"
   * the control (for example by pressing the "enter" key).
   */
  void SetCallback(std::function<void()> _callback) noexcept {
    callback = std::move(_callback);
  }

  void SetCursor(unsigned cursor);

  void SetInvalid();

  void SetValue(int value);
  void SetValue(unsigned value);
  void SetValue(double value);
  void SetValue(RoughTime value);
  void SetValue(Angle value);
  void SetValue(BrokenDate value);
  void SetValue(RadioFrequency value);

  [[gnu::pure]]
  int GetIntegerValue() const;

  [[gnu::pure]]
  unsigned GetUnsignedValue() const;

  [[gnu::pure]]
  double GetDoubleValue() const;

  [[gnu::pure]]
  RoughTime GetTimeValue() const;

  [[gnu::pure]]
  BrokenDate GetDateValue() const;

  [[gnu::pure]]
  Angle GetAngleValue() const;

  void SetLatitude(Angle value, CoordinateFormat format);
  void SetLongitude(Angle value, CoordinateFormat format);

  [[gnu::pure]]
  Angle GetGeoAngle(CoordinateFormat format) const;

  [[gnu::pure]]
  Angle GetLatitude(CoordinateFormat format) const;

  [[gnu::pure]]
  Angle GetLongitude(CoordinateFormat format) const;
  
  [[gnu::pure]]
  RadioFrequency GetRadioFrequency() const;

protected:
  [[gnu::pure]]
  bool IsSigned() const {
    return columns[0].IsSign();
  }

  [[gnu::pure]]
  bool IsNegative() const {
    return columns[0].IsSign() && columns[0].IsNegative();
  }

  /**
   * Find the first column with that contains a decimal point.
   */
  [[gnu::pure]]
  int FindDecimalPoint() const;

  /**
   * Find the next column to the left (including i) that is numerical.
   */
  [[gnu::pure]]
  int FindNumberLeft(int i) const;

  /**
   * Find the next column to the left (including i) that is editable.
   */
  [[gnu::pure]]
  int FindEditableLeft(int i) const;

  /**
   * Find the next column to the right (including i) that is editable.
   */
  [[gnu::pure]]
  int FindEditableRight(unsigned i) const;

  [[gnu::pure]]
  unsigned GetPositiveInteger() const;

  [[gnu::pure]]
  double GetPositiveFractional() const;

  void IncrementColumn(unsigned i);
  void DecrementColumn(unsigned i);

  [[gnu::pure]]
  int FindColumnAt(unsigned x) const;

private:
  void SetDigits(double degrees, CoordinateFormat format, bool isLatitude);

protected:
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};
