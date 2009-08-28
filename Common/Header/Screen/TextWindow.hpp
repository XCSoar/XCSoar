#ifndef XCSOAR_SCREEN_TEXT_WINDOW_HXX
#define XCSOAR_SCREEN_TEXT_WINDOW_HXX

#include "Screen/Window.hpp"

class TextWidget : public Window {
public:
  void set(ContainerWindow &parent, unsigned left, unsigned top,
           unsigned width, unsigned height,
           bool center = false, bool notify = false, bool show = true,
           bool tabstop = false, bool border = false);

  void set_text(const TCHAR *text) {
    ::SetWindowText(hWnd, text);
  }
};

#endif
