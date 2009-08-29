#include "Screen/PaintWindow.hpp"

#include <assert.h>

WindowCanvas::WindowCanvas(HWND _wnd, unsigned width, unsigned height)
  :Canvas(::GetDC(_wnd), width, height), wnd(_wnd) {}

WindowCanvas::~WindowCanvas()
{
  reset();
}

void WindowCanvas::set(HWND _wnd, unsigned _width, unsigned _height)
{
  assert(_wnd != NULL);

  reset();
  Canvas::set(GetDC(_wnd), _width, _height);
}

void WindowCanvas::reset()
{
  if (dc != NULL)
    ::ReleaseDC(wnd, dc);
}

void
Widget::set(ContainerWindow *parent, unsigned left, unsigned top,
            unsigned width, unsigned height,
            bool center, bool notify, bool show,
            bool tabstop, bool border)
{
  canvas.reset();

  Window::set(parent, TEXT("STATIC"), TEXT(" "),
              left, top, width, height,
              center, notify, show, tabstop, border);

  if (!canvas.defined())
    canvas.set(hWnd, width, height);
}

void
Widget::set(ContainerWindow &parent, unsigned left, unsigned top,
            unsigned width, unsigned height,
            bool center, bool notify, bool show,
            bool tabstop, bool border)
{
  set(&parent, left, top, width, height,
      center, notify, show, tabstop, border);
}

void
Widget::created(HWND _hWnd)
{
  assert(!canvas.defined());

  Window::created(_hWnd);
  canvas.set(hWnd, 1, 1);
}

void
Widget::reset()
{
  canvas.reset();
  Window::reset();
}
