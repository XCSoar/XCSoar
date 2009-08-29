#ifndef XCSOAR_SCREEN_PAINT_WINDOW_HXX
#define XCSOAR_SCREEN_PAINT_WINDOW_HXX

#include "Screen/Canvas.hpp"
#include "Screen/Window.hpp"

class WindowCanvas : public Canvas {
protected:
  HWND wnd;

public:
  WindowCanvas() {}
  WindowCanvas(HWND _wnd, unsigned width, unsigned height);
  ~WindowCanvas();

  void set(HWND _wnd, unsigned _width, unsigned _height);
  void reset();
};

class ContainerWindow;

class Widget : public Window {
private:
  WindowCanvas canvas;

public:
  void set(ContainerWindow &parent, LPCTSTR cls,
           unsigned left, unsigned top,
           unsigned width, unsigned height) {
    Window::set(&parent, cls, NULL, left, top, width, height);
  }

  void set(ContainerWindow *parent, unsigned left, unsigned top,
           unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void set(ContainerWindow &parent, unsigned left, unsigned top,
           unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void created(HWND _hWnd);

  void reset();

  void resize(unsigned width, unsigned height) {
    canvas.resize(width, height);
  }

  Canvas &get_canvas() {
    return canvas;
  }

  const Canvas &get_canvas() const {
    return canvas;
  }

  unsigned get_width() const {
    return canvas.get_width();
  }

  unsigned get_height() const {
    return canvas.get_height();
  }

  unsigned get_left() const {
    return 0;
  }

  unsigned get_top() const {
    return 0;
  }

  unsigned get_right() const {
    return get_left() + get_width();
  }

  unsigned get_bottom() const {
    return get_top() + get_height();
  }

  unsigned get_hmiddle() const {
    return (get_left() + get_right()) / 2;
  }

  unsigned get_vmiddle() const {
    return (get_top() + get_bottom()) / 2;
  }

  /**
   * Ensures that the specified rectangle is updated on the physical
   * screen.
   */
  void update(const RECT &rect) {
    ::InvalidateRect(hWnd, &rect, false);
  }

  void update() {
    ::UpdateWindow(hWnd);
    // duplicate in MainWindow
  }

  HDC BeginPaint(PAINTSTRUCT *ps) {
    return ::BeginPaint(hWnd, ps);
  }

  void EndPaint(PAINTSTRUCT *ps) {
    ::EndPaint(hWnd, ps);
  }
};

#endif
