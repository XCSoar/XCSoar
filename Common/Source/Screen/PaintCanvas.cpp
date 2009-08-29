#include "Screen/PaintCanvas.hpp"
#include "Screen/PaintWindow.hpp"

PaintCanvas::PaintCanvas(const Widget &widget, HWND _hWnd)
  :hWnd(_hWnd)
{
  HDC hDC = ::BeginPaint(hWnd, &ps);
  set(hDC, widget.get_width(), widget.get_height());
}

PaintCanvas::~PaintCanvas()
{
  DeleteDC(dc);
  ::EndPaint(hWnd, &ps);
}
