#ifndef XCSOAR_SCREEN_PEN_HPP
#define XCSOAR_SCREEN_PEN_HPP

#include "Screen/Color.hpp"

class Pen {
public:
  enum style {
    SOLID = PS_SOLID,
    DASH = PS_DASH,
    BLANK = PS_NULL
  };

protected:
  HPEN pen;

public:
  Pen():pen(NULL) {}
  Pen(enum style style, unsigned width, const Color c):pen(NULL) {
    set(style, width, c);
  }
  Pen(unsigned width, Color c):pen(NULL) {
    set(width, c);
  }
  ~Pen() { reset(); }

public:
  void set(enum style style, unsigned width, const Color c);
  void set(unsigned width, const Color c);
  void reset();

  bool defined() const {
    return pen != NULL;
  }

  HPEN native() const {
    return pen;
  }
};

#endif
