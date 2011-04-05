/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/TopWindow.hpp"
#include "Screen/GDI/Event.hpp"

TopWindow::TopWindow()
  :hSavedFocus(NULL)
{
#ifdef _WIN32_WCE
  task_bar = NULL;
#endif

#ifdef HAVE_AYGSHELL_DLL
  memset(&s_sai, 0, sizeof(s_sai));
  s_sai.cbSize = sizeof(s_sai);
#endif
}

bool
TopWindow::find(const TCHAR *cls, const TCHAR *text)
{
  HWND h = FindWindow(cls, text);
  if (h != NULL)
      SetForegroundWindow((HWND)((ULONG) h | 0x00000001));

  return h != NULL;
}

void
TopWindow::set(const TCHAR *cls, const TCHAR *text,
                int left, int top, unsigned width, unsigned height)
{
  WindowStyle style;
  style.popup();

  Window::set(NULL, cls, text, left, top, width, height, style);
}

#ifdef _WIN32_WCE

void
TopWindow::reset()
{
  ContainerWindow::reset();

  if (task_bar != NULL) {
    /* restore the task bar */
    ::ShowWindow(task_bar, SW_SHOW);
    task_bar = NULL;
  }
}

#endif

void
TopWindow::full_screen()
{
  ::SetForegroundWindow(hWnd);
#ifndef _WIN32_WCE
  show_on_top();
#else

  bool success = false;
#ifdef HAVE_AYGSHELL_DLL
  success = ayg_shell_dll.SHFullScreen(hWnd, SHFS_HIDETASKBAR|
                                       SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
  if (!success) {
    /* hack: on Windows CE Core, there is no aygshell.dll; try to
       manually hide the task bar window */
    task_bar = ::FindWindow(_T("HHTaskBar"), _T(""));
    if (task_bar != NULL) {
      if (::IsWindowVisible(task_bar))
        ::ShowWindow(task_bar, SW_HIDE);
      else
        task_bar = NULL;
    }
  }

  ::SetWindowPos(hWnd, HWND_TOP, 0, 0,
                 GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN),
                 SWP_SHOWWINDOW|SWP_NOOWNERZORDER);
#endif
}

bool
TopWindow::on_activate()
{
  if (hSavedFocus != NULL && ::IsWindow(hSavedFocus) &&
      ::IsWindowVisible(hSavedFocus) && ::IsWindowEnabled(hSavedFocus)) {
    /* restore the keyboard focus to the control which was previously
       focused */
    ::SetFocus(hSavedFocus);
    return true;
  }

  return false;
}

bool
TopWindow::on_deactivate()
{
  /* remember the currently focused control */
  hSavedFocus = ::GetFocus();
  if (hSavedFocus != NULL && !identify_descendant(hSavedFocus))
    hSavedFocus = NULL;

  return false;
}

LRESULT
TopWindow::on_message(HWND _hWnd, UINT message,
                      WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_ACTIVATE:
#ifdef HAVE_AYGSHELL_DLL
    ayg_shell_dll.SHHandleWMActivate(_hWnd, wParam, lParam, &s_sai, FALSE);
#endif

    if (wParam == WA_INACTIVE ? on_deactivate() : on_activate())
      return true;
    break;

  case WM_SETTINGCHANGE:
#ifdef HAVE_AYGSHELL_DLL
    ayg_shell_dll.SHHandleWMSettingChange(_hWnd, wParam, lParam, &s_sai);
#endif
    break;
  };
  return ContainerWindow::on_message(_hWnd, message, wParam, lParam);
}

int
TopWindow::event_loop()
{
  EventLoop loop;
  MSG msg;
  while (loop.get(msg))
    loop.dispatch(msg);

  return msg.wParam;
}

void
TopWindow::post_quit()
{
  ::PostQuitMessage(0);
}
