#ifndef XCSOAR_SCREEN_MAIN_WINDOW_HXX
#define XCSOAR_SCREEN_MAIN_WINDOW_HXX

#include "Screen/ContainerWindow.hpp"

class MainWindow : public ContainerWindow {
public:
  bool find(LPCTSTR cls, LPCTSTR text);

  void set(LPCTSTR cls, LPCTSTR text,
           unsigned left, unsigned top,
           unsigned width, unsigned height);

  void full_screen();

  void update() {
    ::UpdateWindow(hWnd);
  }

  void close() {
    ::SendMessage(hWnd, WM_CLOSE, 0, 0);
  }
};

#endif
