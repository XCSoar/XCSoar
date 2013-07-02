/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Event/GDI/Globals.hpp"
#include "Event/GDI/Event.hpp"
#include "Event/GDI/Loop.hpp"
#include "Event/GDI/Queue.hpp"

bool
TopWindow::find(const TCHAR *cls, const TCHAR *text)
{
  HWND h = FindWindow(cls, text);
  if (h != NULL) {
#ifdef _WIN32_WCE
    /* restore and reactivate the last active child window */
    /* On MSDN, this flag is only documented on Windows CE, and not in
       all versions of the SetForegroundWindow() documentation.  The
       cast below is copied from MSDN example code, but it's wrong:
       casting a pointer to ULONG doesn't work on 64 bit platforms.
       There is no 64 bit flavour of Windows CE, so that's good enough
       for us. */
    h = (HWND)((ULONG) h | 0x00000001);
#endif

    SetForegroundWindow(h);
  }

  return h != NULL;
}

void
TopWindow::Create(const TCHAR *cls, const TCHAR *text, PixelSize size,
                  TopWindowStyle style)
{
  hSavedFocus = nullptr;

#ifdef _WIN32_WCE
  task_bar = NULL;
#endif

#ifdef HAVE_AYGSHELL_DLL
  memset(&s_sai, 0, sizeof(s_sai));
  s_sai.cbSize = sizeof(s_sai);
#endif

#ifdef _WIN32_WCE
  /* full-screen on Windows CE */
  const RasterPoint position(0, 0);
#else
  const RasterPoint position(CW_USEDEFAULT, CW_USEDEFAULT);
#endif

  Window::Create(NULL, cls, text, PixelRect(position, size), style);
}

#ifdef _WIN32_WCE

void
TopWindow::Destroy()
{
  ContainerWindow::Destroy();

  if (task_bar != NULL) {
    /* restore the task bar */
    ::ShowWindow(task_bar, SW_SHOW);
    task_bar = NULL;
  }
}

#endif

void
TopWindow::CancelMode()
{
  HWND focus = ::GetFocus();
  if (focus != NULL)
    ::SendMessage(focus, WM_CANCELMODE, 0, 0);
}


void
TopWindow::Fullscreen()
{
  ::SetForegroundWindow(hWnd);
#ifndef _WIN32_WCE
  ShowOnTop();
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

void
TopWindow::Refresh()
{
  EventQueue::HandlePaintMessages();
}

bool
TopWindow::OnActivate()
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
TopWindow::OnDeactivate()
{
  /* remember the currently focused control */
  hSavedFocus = ::GetFocus();
  if (hSavedFocus != NULL && !IdentifyDescendant(hSavedFocus))
    hSavedFocus = NULL;

  return false;
}

bool
TopWindow::OnClose()
{
  return false;
}

LRESULT
TopWindow::OnMessage(HWND _hWnd, UINT message,
                      WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_CLOSE:
    if (OnClose())
      /* true returned: message was handled */
      return 0;
    break;

  case WM_ACTIVATE:
#ifdef HAVE_AYGSHELL_DLL
    ayg_shell_dll.SHHandleWMActivate(_hWnd, wParam, lParam, &s_sai, FALSE);
#endif

    if (wParam == WA_INACTIVE ? OnDeactivate() : OnActivate())
      return true;
    break;

  case WM_SETTINGCHANGE:
#ifdef HAVE_AYGSHELL_DLL
    ayg_shell_dll.SHHandleWMSettingChange(_hWnd, wParam, lParam, &s_sai);
#endif
    break;
  };
  return ContainerWindow::OnMessage(_hWnd, message, wParam, lParam);
}

int
TopWindow::RunEventLoop()
{
  EventLoop loop(*event_queue);
  Event event;
  while (loop.Get(event))
    loop.Dispatch(event);

  return event.msg.wParam;
}

void
TopWindow::PostQuit()
{
  ::PostQuitMessage(0);
}
