#ifndef XCSOAR_SCREEN_EDIT_WINDOW_HXX
#define XCSOAR_SCREEN_EDIT_WINDOW_HXX

#include "Screen/Window.hpp"

class EditWidget : public Window {
public:
  void set(ContainerWindow &parent, int left, int top,
           unsigned width, unsigned height,
           bool multiline = false);

  void set_text(const TCHAR *text) {
    ::SetWindowText(hWnd, text);
  }

  void set_read_only(bool value) {
    ::SendMessage(hWnd, EM_SETREADONLY, (WPARAM)(BOOL)value, 0L);
  }

  void set_selection(int start, int end) {
    ::SendMessage(hWnd, EM_SETSEL, (WPARAM)start, (LPARAM)end);
  }

  void set_selection() {
    set_selection(0, -1);
  }
};

#endif
