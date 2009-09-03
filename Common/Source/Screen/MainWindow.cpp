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

#include "Screen/MainWindow.hpp"
#include "resource.h"

#if (((UNDER_CE >= 300)||(_WIN32_WCE >= 0x0300)) && (WINDOWSPC<1))
#define HAVE_ACTIVATE_INFO
#endif

#ifdef HAVE_ACTIVATE_INFO
#include <aygshell.h>
static SHACTIVATEINFO s_sai;
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

bool 
MainWindow::register_class(HINSTANCE hInstance, const TCHAR* szWindowClass)
{
  WNDCLASS wc;

  wc.style                      = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra                 = 0;
#if (WINDOWSPC>0)
  wc.cbWndExtra = 0;
#else
  WNDCLASS dc;
  GetClassInfo(hInstance,TEXT("DIALOG"),&dc);
  wc.cbWndExtra                 = dc.cbWndExtra ;
#endif
  wc.hInstance                  = hInstance;
#if defined(GNAV) && !defined(PCGNAV)
  wc.hIcon = NULL;
#else
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
#endif
  wc.hCursor                    = 0;
  wc.hbrBackground              = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName               = 0;
  wc.lpszClassName              = szWindowClass;

  return (RegisterClass(&wc)!= FALSE);
}

///////////////////////////////////////////////////////////////////////////
// Windows event handlers

#include "Protection.hpp"
#include "InfoBoxManager.h"
#include "Message.h"
#include "Interface.hpp"
#include "ButtonLabel.h"
#include "Screen/Graphics.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"

bool
MainWindow::on_command(HWND wmControl, unsigned id, unsigned code)
{
  if (wmControl && globalRunningEvent.test()) {

    full_screen();

    if (InfoBoxManager::Click(wmControl)) {
      return true; // don't continue processing..
    }

    Message::CheckTouch(wmControl);
    
    if (ButtonLabel::CheckButtonPress(wmControl)) {
      return true; // don't continue processing..
    }
  }

  return ContainerWindow::on_command(wmControl, id, code);
}


LRESULT MainWindow::on_colour(HDC hdc, int wdata)
{
  switch(wdata) {
  case 0:
    SetBkColor(hdc, MapGfx.ColorUnselected);
    SetTextColor(hdc, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 1:
    SetBkColor(hdc, MapGfx.ColorSelected);
    SetTextColor(hdc, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.infoSelectedBrush.native();
  case 2:
    SetBkColor(hdc, MapGfx.ColorUnselected);
    SetTextColor(hdc, MapGfx.ColorWarning);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 3:
    SetBkColor(hdc, MapGfx.ColorUnselected);
    SetTextColor(hdc, MapGfx.ColorOK);
    return (LRESULT)MapGfx.infoUnselectedBrush.native();
  case 4:
    // black on light green
    SetBkColor(hdc, MapGfx.ColorButton);
    SetTextColor(hdc, MapGfx.ColorBlack);
    return (LRESULT)MapGfx.buttonBrush.native();
  case 5:
    // grey on light green
    SetBkColor(hdc, MapGfx.ColorButton);
    SetTextColor(hdc, MapGfx.ColorMidGrey);
    return (LRESULT)MapGfx.buttonBrush.native();
  }
}

bool MainWindow::on_timer(void)
{
  if (globalRunningEvent.test()) {
    AfterStartup();
#ifdef _SIM_
    SIMProcessTimer();
#else
    ProcessTimer();
#endif
  }
  return true;
}

bool MainWindow::on_create(void)
{
  // strange, this never gets called..
#ifdef HAVE_ACTIVATE_INFO
  memset (&s_sai, 0, sizeof (s_sai));
  s_sai.cbSize = sizeof (s_sai);
#endif
  if (_timer_id == 0) {
    _timer_id = SetTimer(hWnd,1000,500,NULL); // 2 times per second
  }
  return true;
}

void MainWindow::install_timer(void) {
}

bool MainWindow::on_destroy(void) {
  PaintWindow::on_destroy();

  PostQuitMessage(0);
  return true;
}

bool MainWindow::on_close() {
  if (CheckShutdown()) {
    if(_timer_id) {
      ::KillTimer(hWnd, _timer_id);
      _timer_id = 0;
    }
    Shutdown();
  }
  return true;
}


LRESULT MainWindow::on_message(HWND _hWnd, UINT message,
			       WPARAM wParam, LPARAM lParam) {
  switch (message) {
    /*
  case WM_CTLCOLORSTATIC:
    return on_colour((HDC)wParam, get_userdata((HWND)lParam));
    break;
    */
  case WM_SETFOCUS:
    InfoBoxManager::Focus();
    break;
  case WM_ACTIVATE:
    if(LOWORD(wParam) != WA_INACTIVE) {
      full_screen();
    }
#ifdef HAVE_ACTIVATE_INFO
    SHHandleWMActivate(_hWnd, wParam, lParam, &s_sai, FALSE);
#endif
    break;
  case WM_SETTINGCHANGE:
#ifdef HAVE_ACTIVATE_INFO
    SHHandleWMSettingChange(_hWnd, wParam, lParam, &s_sai);
#endif
    break;
  case WM_TIMER:
    if (on_timer()) return 0;
    break;
  };
  return ContainerWindow::on_message(_hWnd, message, wParam, lParam);
}

