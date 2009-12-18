/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

TopWindow::TopWindow()
#ifndef ENABLE_SDL
  :hSavedFocus(NULL)
#endif
{
#ifdef HAVE_ACTIVATE_INFO
  memset(&s_sai, 0, sizeof(s_sai));
  s_sai.cbSize = sizeof(s_sai);
#endif
}

bool
TopWindow::find(const TCHAR *cls, const TCHAR *text)
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
TopWindow::set(const TCHAR *cls, const TCHAR *text,
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

  ContainerWindow * top_parent = NULL;
  DWORD 			style = (DWORD)(WS_SYSMENU|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);

  #if defined(sim_top_window_override)
	extern void sim_top_window_override(ContainerWindow * &parent, int &left, int &top, unsigned &width, unsigned &height, DWORD &style);
	sim_top_window_override(top_parent, left, top, width, height, style);
  #endif

  Window::set(top_parent, cls, text, left, top, width, height, style);

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
  show_on_top();
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
#ifndef ENABLE_SDL
  if (hSavedFocus != NULL && ::IsWindow(hSavedFocus) &&
      ::IsWindowVisible(hSavedFocus) && ::IsWindowEnabled(hSavedFocus)) {
    /* restore the keyboard focus to the control which was previously
       focused */
    ::SetFocus(hSavedFocus);
    return true;
  }
#endif

  return false;
}

bool
TopWindow::on_deactivate()
{
#ifndef ENABLE_SDL
  /* remember the currently focused control */
  hSavedFocus = ::GetFocus();
  if (hSavedFocus != NULL && !identify_descendant(hSavedFocus))
    hSavedFocus = NULL;
#endif

  return false;
}

void
TopWindow::post_quit()
{
#ifdef ENABLE_SDL
  SDL_Event event;
  event.type = SDL_QUIT;
  ::SDL_PushEvent(&event);
#else
  ::PostQuitMessage(0);
#endif
}

#ifdef ENABLE_SDL

bool
TopWindow::on_event(const SDL_Event &event)
{
  switch (event.type) {
  case SDL_MOUSEMOTION:
    // XXX keys
    return on_mouse_move(event.motion.x, event.motion.y, 0);

  case SDL_MOUSEBUTTONDOWN:
    return on_mouse_down(event.button.x, event.button.y);

  case SDL_MOUSEBUTTONUP:
    return on_mouse_up(event.button.x, event.button.y);
  }

  return false;
}

#else /* !ENABLE_SDL */

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

    if (event.type >= SDL_USEREVENT && event.type <= SDL_NUMEVENTS-1 &&
        event.user.data1 != NULL) {
      Window *window = (Window *)event.user.data1;
      window->on_user(event.type - SDL_USEREVENT);
    } else
      on_event(event);
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
