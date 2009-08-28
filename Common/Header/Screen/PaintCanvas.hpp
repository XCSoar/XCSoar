#ifndef XCSOAR_SCREEN_PAINT_CANVAS_HPP
#define XCSOAR_SCREEN_PAINT_CANVAS_HPP

#include "Screen/Canvas.hpp"

class Widget;

class PaintCanvas : public Canvas {
private:
  HWND hWnd;
  PAINTSTRUCT ps;

public:
  PaintCanvas(const Widget &widget, HWND _hWnd);
  ~PaintCanvas();
};

#endif
