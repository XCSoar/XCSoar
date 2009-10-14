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

#ifdef ENABLE_SDL
#include <wcecompat/ts_string.h>
#else /* !ENABLE_SDL */
#include "Interface.hpp" /* for XCSoarInterface::hInst */
#if defined(GNAV) && !defined(PCGNAV)
#include "resource.h" /* for IDI_XCSOARSWIFT */
#endif
#endif /* !ENABLE_SDL */

#ifdef ENABLE_SDL

void
TopCanvas::set()
{
  Canvas::set(::SDL_SetVideoMode(640, 480, 0, SDL_HWSURFACE|SDL_ANYFORMAT));
}

void
TopCanvas::full_screen()
{
  ::SDL_WM_ToggleFullScreen(surface);
}

#endif /* ENABLE_SDL */

TopWindow::TopWindow() {
#ifdef HAVE_ACTIVATE_INFO
  memset(&s_sai, 0, sizeof(s_sai));
  s_sai.cbSize = sizeof(s_sai);
#endif
}

bool
TopWindow::find(LPCTSTR cls, LPCTSTR text)
{
#ifdef ENABLE_SDL
  return false; // XXX
#else /* !ENABLE_SDL */
  HWND h = FindWindow(cls, text);
  if (h != NULL)
      SetForegroundWindow((HWND)((ULONG) h | 0x00000001));

  return h != NULL;
#endif /* !ENABLE_SDL */
}

void
TopWindow::set(LPCTSTR cls, LPCTSTR text,
                int left, int top, unsigned width, unsigned height)
{
#ifdef ENABLE_SDL
  int ret;

  ret = ::SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
  if (ret != 0)
    fprintf(stderr, "SDL_Init() has failed\n");

  ret = ::TTF_Init();
  if (ret != 0)
    fprintf(stderr, "TTF_Init() has failed: %s\n", TTF_GetError());

  screen.set();
  ContainerWindow::set(NULL, 0, 0, width, height);

  char text2[512];
  unicode2ascii(text, text2);
  ::SDL_WM_SetCaption(text2, NULL);
#else /* !ENABLE_SDL */
  Window::set(NULL, cls, text, left, top, width, height,
              (DWORD)(WS_SYSMENU|WS_CLIPCHILDREN|WS_CLIPSIBLINGS));

#if defined(GNAV) && !defined(PCGNAV)
  // TODO code: release the handle?
  HANDLE hTmp = LoadIcon(XCSoarInterface::hInst,
                         MAKEINTRESOURCE(IDI_XCSOARSWIFT));
  SendMessage(hWnd, WM_SETICON,
	      (WPARAM)ICON_BIG, (LPARAM)hTmp);
  SendMessage(hWnd, WM_SETICON,
	      (WPARAM)ICON_SMALL, (LPARAM)hTmp);
#endif
#endif /* !ENABLE_SDL */
}

void
TopWindow::full_screen()
{
#ifdef ENABLE_SDL
  screen.full_screen();
#else /* !ENABLE_SDL */
  ::SetForegroundWindow(hWnd);
#ifdef WINDOWSPC
  ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                 SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
#else
#if !defined(CECORE) && !defined(GNAV)
  ::SHFullScreen(hWnd, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
  ::SetWindowPos(hWnd, HWND_TOP, 0, 0,
                 GetSystemMetrics(SM_CXSCREEN),
                 GetSystemMetrics(SM_CYSCREEN),
                 SWP_SHOWWINDOW);
#endif
#endif /* !ENABLE_SDL */
}

#ifdef ENABLE_SDL

void
TopWindow::expose(const RECT &rect) {
  ContainerWindow::expose(rect);
  screen.copy(canvas);
  screen.expose();
}

void
TopWindow::expose() {
  ContainerWindow::expose();
  screen.copy(canvas);
  screen.expose();
}

#endif /* ENABLE_SDL */

bool
TopWindow::on_activate()
{
  return false;
}

bool
TopWindow::on_deactivate()
{
  return false;
}

#ifndef ENABLE_SDL
LRESULT TopWindow::on_message(HWND _hWnd, UINT message,
			       WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_ACTIVATE:
#ifdef HAVE_ACTIVATE_INFO
    SHHandleWMActivate(_hWnd, wParam, lParam, &s_sai, FALSE);
#endif

    if (wParam == WA_INACTIVE ? on_deactivate() : on_activate())
      return true;
    break;

  case WM_SETTINGCHANGE:
#ifdef HAVE_ACTIVATE_INFO
    SHHandleWMSettingChange(_hWnd, wParam, lParam, &s_sai);
#endif
    break;
  };
  return ContainerWindow::on_message(_hWnd, message, wParam, lParam);
}
#endif /* !ENABLE_SDL */

int
TopWindow::event_loop(unsigned accelerators_id)
{
#ifdef ENABLE_SDL
  SDL_Event event;

  update();

  while (SDL_WaitEvent(&event)) {
    if (event.type == SDL_QUIT)
      break;
  }

  return 0;

#else /* !ENABLE_SDL */

  HACCEL hAccelerators = accelerators_id != 0
    ? ::LoadAccelerators(XCSoarInterface::hInst,
                         MAKEINTRESOURCE(accelerators_id))
    : NULL;

  MSG msg;
  while (::GetMessage(&msg, NULL, 0, 0)) {
    if (hAccelerators == NULL ||
        !::TranslateAccelerator(msg.hwnd, hAccelerators, &msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  return msg.wParam;
#endif /* !ENABLE_SDL */
}
