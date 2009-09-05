/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#if (((UNDER_CE >= 300)||(_WIN32_WCE >= 0x0300)) && !defined(WINDOWSPC))
#define HAVE_ACTIVATE_INFO
#include <aygshell.h>
#endif

TopWindow::TopWindow() {
#ifdef HAVE_ACTIVATE_INFO
  memset(&s_sai, 0, sizeof(s_sai));
  s_sai.cbSize = sizeof(s_sai);
#endif
}

bool
TopWindow::find(LPCTSTR cls, LPCTSTR text)
{
  HWND h = FindWindow(cls, text);
  if (h != NULL)
      SetForegroundWindow((HWND)((ULONG) h | 0x00000001));

  return h != NULL;
}

void
TopWindow::set(LPCTSTR cls, LPCTSTR text,
                int left, int top, unsigned width, unsigned height)
{
  Window::set(NULL, cls, text, left, top, width, height,
              (DWORD)(WS_SYSMENU|WS_CLIPCHILDREN|WS_CLIPSIBLINGS));

#if defined(GNAV) && !defined(PCGNAV)
  // TODO code: release the handle?
  HANDLE hTmp = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
  SendMessage(hWnd, WM_SETICON,
	      (WPARAM)ICON_BIG, (LPARAM)hTmp);
  SendMessage(hWnd, WM_SETICON,
	      (WPARAM)ICON_SMALL, (LPARAM)hTmp);
#endif
}

void
TopWindow::full_screen()
{
  ::SetForegroundWindow(hWnd);
#ifdef WINDOWSPC
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

LRESULT TopWindow::on_message(HWND _hWnd, UINT message,
			       WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_ACTIVATE:
#ifdef HAVE_ACTIVATE_INFO
    SHHandleWMActivate(_hWnd, wParam, lParam, &s_sai, FALSE);
#endif
    break;
  case WM_SETTINGCHANGE:
#ifdef HAVE_ACTIVATE_INFO
    SHHandleWMSettingChange(_hWnd, wParam, lParam, &s_sai);
#endif
    break;
  };
  return ContainerWindow::on_message(_hWnd, message, wParam, lParam);
}
