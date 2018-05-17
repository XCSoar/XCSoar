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

#ifndef XCSOAR_SCREEN_PORTABLE_COLOR_HPP
#define XCSOAR_SCREEN_PORTABLE_COLOR_HPP

#include <stdint.h>

/**
 * Represents a device-independent color class that stores 8 bits for
 * each channel, ordered RGB.
 */
class RGB8Color {
  uint8_t r, g, b;

public:
  RGB8Color() = default;

  constexpr RGB8Color(uint8_t _r, uint8_t _g, uint8_t _b)
    :r(_r), g(_g), b(_b) {}

  constexpr uint8_t Red() const {
    return r;
  }

  constexpr uint8_t Green() const {
    return g;
  }

  constexpr uint8_t Blue() const {
    return b;
  }

  constexpr bool operator ==(const RGB8Color other) const {
    return r == other.r && g == other.g && b == other.b;
  }

  constexpr bool operator !=(const RGB8Color other) const {
    return !(*this == other);
  }

  constexpr RGB8Color Invert() const {
    return RGB8Color(~r, ~g, ~b);
  }

  constexpr RGB8Color Darken() const {
    return RGB8Color(r >> 1, g >> 1, b >> 1);
  }

  constexpr RGB8Color Lighten() const {
    return Invert().Darken().Invert();
  }
};

static constexpr RGB8Color RGB8_WHITE = RGB8Color(0xff, 0xff, 0xff);
static constexpr RGB8Color RGB8_BLACK = RGB8Color(0x00, 0x00, 0x00);
static constexpr RGB8Color RGB8_GRAY = RGB8Color(0x80, 0x80, 0x80);
static constexpr RGB8Color RGB8_VERY_LIGHT_GRAY = RGB8Color(0xd8, 0xd8, 0xd8);
static constexpr RGB8Color RGB8_LIGHT_GRAY = RGB8Color(0xc0, 0xc0, 0xc0);
static constexpr RGB8Color RGB8_DARK_GRAY = RGB8Color(0x40, 0x40, 0x40);
static constexpr RGB8Color RGB8_RED = RGB8Color(0xff, 0x00, 0x00);
static constexpr RGB8Color RGB8_GREEN = RGB8Color(0x00, 0xff, 0x00);
static constexpr RGB8Color RGB8_BLUE = RGB8Color(0x00, 0x00, 0xff);
static constexpr RGB8Color RGB8_YELLOW = RGB8Color(0xff, 0xff, 0x00);
static constexpr RGB8Color RGB8_CYAN = RGB8Color(0x00, 0xff, 0xff);
static constexpr RGB8Color RGB8_MAGENTA = RGB8Color(0xff, 0x00, 0xff);
static constexpr RGB8Color RGB8_ORANGE = RGB8Color(0xff, 0xa2, 0x00);
static constexpr RGB8Color RGB8_BROWN = RGB8Color(0xb7, 0x64, 0x1e);

/**
 * Represents a device-independent color class that stores 8 bits for
 * each channel, ordered BGR.
 */
class BGR8Color {
  uint8_t b, g, r;

public:
  BGR8Color() = default;

  constexpr BGR8Color(uint8_t _r, uint8_t _g, uint8_t _b)
    :b(_b), g(_g), r(_r) {}

  constexpr uint8_t Red() const {
    return r;
  }

  constexpr uint8_t Green() const {
    return g;
  }

  constexpr uint8_t Blue() const {
    return b;
  }

  constexpr bool operator ==(const BGR8Color other) const {
    return r == other.r && g == other.g && b == other.b;
  }

  constexpr bool operator !=(const BGR8Color other) const {
    return !(*this == other);
  }
};

/**
 * Represents a device-independent color class that stores 8 bits for
 * each channel, ordered BGRA.
 */
class BGRA8Color {
  BGR8Color base;
  uint8_t a;

public:
  BGRA8Color() = default;

  constexpr BGRA8Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a=0xff)
    :base(_r, _g, _b), a(_a) {}

  constexpr uint8_t Red() const {
    return base.Red();
  }

  constexpr uint8_t Green() const {
    return base.Green();
  }

  constexpr uint8_t Blue() const {
    return base.Blue();
  }

  constexpr uint8_t Alpha() const {
    return a;
  }

  constexpr bool operator ==(const BGRA8Color other) const {
    return base == other.base && a == other.a;
  }

  constexpr bool operator !=(const BGRA8Color other) const {
    return !(*this == other);
  }
};

/**
 * Represents a device-independent color class that stores 5 bits for
 * each channel.  The most significant bit is unused.
 */
class RGB555Color {
  uint16_t value;

public:
  RGB555Color() = default;

  constexpr RGB555Color(uint8_t _r, uint8_t _g, uint8_t _b)
    :value(((_r & 0xf8) << 7) |
           ((_g & 0xf8) << 2) |
           (_b >> 3)) {}

  constexpr uint16_t GetNativeValue() const {
    return value;
  }
};

/**
 * Represents a device-independent color class that stores 5/6/5 bits
 * for each channel.
 */
class RGB565Color {
  uint16_t value;

public:
  RGB565Color() = default;

  constexpr RGB565Color(uint8_t _r, uint8_t _g, uint8_t _b)
    :value(((_r & 0xf8) << 8) |
           ((_g & 0xfc) << 3) |
           (_b >> 3)) {}

  constexpr uint16_t GetNativeValue() const {
    return value;
  }
};

class Luminosity8 {
  uint8_t value;

  constexpr
  static uint8_t FromRGB(uint8_t r, uint8_t g, uint8_t b) {
    return (r * 2126 + g * 7152 + b * 722 + 5000) / 10000;
  }

public:
  Luminosity8() = default;

  constexpr Luminosity8(uint8_t _value)
    :value(_value) {}

  constexpr Luminosity8(uint8_t r, uint8_t g, uint8_t b)
    :value(FromRGB(r, g, b)) {}

  explicit constexpr Luminosity8(RGB8Color color)
    :value(FromRGB(color.Red(), color.Green(), color.Blue())) {}

  constexpr uint8_t GetLuminosity() const {
    return value;
  }

  constexpr bool operator ==(const Luminosity8 other) const {
    return value == other.value;
  }

  constexpr bool operator !=(const Luminosity8 other) const {
    return !(*this == other);
  }
};

#endif
