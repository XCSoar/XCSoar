#include "Screen/MainWindow.hpp"

#ifndef CECORE
#include <aygshell.h>
#endif

bool
MainWindow::find(LPCTSTR cls, LPCTSTR text)
{
  HWND h = FindWindow(cls, text);
  if (h != NULL)
      SetForegroundWindow((HWND)((ULONG) h | 0x00000001));

  return h != NULL;
}

void
MainWindow::set(LPCTSTR cls, LPCTSTR text,
                unsigned left, unsigned top,
                unsigned width, unsigned height)
{
  Window::set(NULL, cls, text, left, top, width, height,
              (DWORD)(WS_SYSMENU|WS_CLIPCHILDREN|WS_CLIPSIBLINGS));
}

void
MainWindow::full_screen()
{
  ::SetForegroundWindow(hWnd);
#if (WINDOWSPC>0)
  ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                 SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
#else
#ifndef CECORE
  ::SHFullScreen(hWnd, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
  ::SetWindowPos(hWnd, HWND_TOP, 0, 0,
                 GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN),
                 SWP_SHOWWINDOW);
#endif
}
