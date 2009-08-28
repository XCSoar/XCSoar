#ifndef XCSOAR_SCREEN_BRUSH_HPP
#define XCSOAR_SCREEN_BRUSH_HPP

#include "Screen/Color.hpp"

class Bitmap;

class Brush {
protected:
  HBRUSH brush;

public:
  Brush():brush(NULL) {}
  Brush(const Color c):brush(NULL) {
    set(c);
  }
  ~Brush() { reset(); }

public:
  void set(const Color c);
  void set(const Bitmap &bitmap);
  void reset();

  bool defined() const {
    return brush != NULL;
  }

  HBRUSH native() const {
    return brush;
  }
};

#endif
