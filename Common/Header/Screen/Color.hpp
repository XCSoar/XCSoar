#ifndef XCSOAR_SCREEN_COLOR_HPP
#define XCSOAR_SCREEN_COLOR_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Color {
  COLORREF value;

  Color():value(RGB(0, 0, 0)) {}
  Color(COLORREF c):value(c) {}
  Color(int r, int g, int b):value(RGB(r, g, b)) {}

  unsigned char red() const {
    return GetRValue(value);
  }

  unsigned char green() const {
    return GetGValue(value);
  }

  unsigned char blue() const {
    return GetBValue(value);
  }

  Color &operator =(COLORREF c) {
    value = c;
    return *this;
  }

  operator COLORREF() const {
    return value;
  }
};

static inline bool
operator ==(const Color a, const Color b)
{
  return a.value == b.value;
}

static inline bool
operator !=(const Color a, const Color b)
{
  return !(a == b);
}

struct HWColor {
  COLORREF value;

  HWColor():value(0) {}
  HWColor(COLORREF c):value(c) {}

  operator COLORREF() const {
    return value;
  }
};

#endif
