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
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Globals.hpp"
#endif

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "PeriodClock.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Event.hpp"
#include "PeriodClock.hpp"
#else
#include "Screen/GDI/Event.hpp"
#if defined(GNAV) && !defined(PCGNAV)
#include "resource.h" /* for IDI_XCSOAR */
#endif
#endif /* !ENABLE_SDL */

TopWindow::TopWindow()
#ifdef ENABLE_SDL
  :invalidated(false)
#else
  :hSavedFocus(NULL)
#endif
{
#ifdef HAVE_AYGSHELL_DLL
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
  WindowStyle style;
  style.popup();

#ifdef ENABLE_SDL
  screen.set();

  /* apply the mode which was chosen by TopCanvas */
  width = screen.get_width();
  height = screen.get_height();

  ContainerWindow::set(NULL, cls, 0, 0, width, height, style);

#ifndef ANDROID
#ifdef _UNICODE
  char text2[_tcslen(text) * 4];
  ::WideCharToMultiByte(CP_UTF8, 0, text, -1, text2, sizeof(text2),
                        NULL, NULL);
#else
  const char *text2 = text;
#endif

  ::SDL_WM_SetCaption(text2, NULL);
#endif
#else /* !ENABLE_SDL */
  Window::set(NULL, cls, text, left, top, width, height, style);
#endif /* !ENABLE_SDL */
}

void
TopWindow::full_screen()
{
#ifdef ENABLE_SDL
  screen.full_screen();
#else /* !ENABLE_SDL */
  ::SetForegroundWindow(hWnd);
#ifndef _WIN32_WCE
  show_on_top();
#else
#ifdef HAVE_AYGSHELL_DLL
  ayg_shell_dll.SHFullScreen(hWnd, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|
                             SHFS_HIDESTARTICON);
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
TopWindow::invalidate()
{
  invalidated_lock.Lock();
  if (invalidated) {
    /* already invalidated, don't send the event twice */
    invalidated_lock.Unlock();
    return;
  }

  invalidated = true;
  invalidated_lock.Unlock();

  /* wake up the event loop */
#ifdef ANDROID
  event_queue->push(Event::NOP);
#else
  /* note that SDL_NOEVENT is not documented, but since we just want
     to wake up without actually sending an event, I hope this works
     on all future SDL versions; if SDL_NOEVENT ever gets remove, I'll
     have to come up with something else */
  SDL_Event event;
  event.type = SDL_NOEVENT;
  ::SDL_PushEvent(&event);
#endif
}

void
TopWindow::expose() {
  on_paint(screen);
  screen.flip();
}

void
TopWindow::refresh()
{
#ifdef ANDROID
  if (android_paused)
    /* the application is paused/suspended, and we don't have an
       OpenGL surface - ignore all drawing requests */
    return;
  else if (android_resumed) {
    /* the application was just resumed */
    android_resumed = false;
    native_view->initSurface();
    screen.set();
  }
#endif

  invalidated_lock.Lock();
  if (!invalidated) {
    invalidated_lock.Unlock();
    return;
  }

  invalidated = false;
  invalidated_lock.Unlock();

  expose();
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
#ifdef ANDROID
  event_queue->push(Event::QUIT);
#elif defined(ENABLE_SDL)
  SDL_Event event;
  event.type = SDL_QUIT;
  ::SDL_PushEvent(&event);
#else
  ::PostQuitMessage(0);
#endif
}

#ifdef ANDROID

bool
TopWindow::on_event(const Event &event)
{
  switch (event.type) {
    Window *w;

  case Event::NOP:
  case Event::QUIT:
  case Event::TIMER:
  case Event::USER:
    break;

  case Event::KEY_DOWN:
    w = get_focused_window();
    if (w == NULL)
      w = this;

    return w->on_key_down(event.param);

  case Event::KEY_UP:
    w = get_focused_window();
    if (w == NULL)
      w = this;

    return w->on_key_up(event.param);

  case Event::MOUSE_MOTION:
    // XXX keys
    return on_mouse_move(event.x, event.y, 0);

  case Event::MOUSE_DOWN:
    static PeriodClock double_click;
    return double_click.check_always_update(300)
      ? on_mouse_down(event.x, event.y)
      : on_mouse_double(event.x, event.y);

  case Event::MOUSE_UP:
    return on_mouse_up(event.x, event.y);
  }

  return false;
}

#elif defined(ENABLE_SDL)

bool
TopWindow::on_event(const SDL_Event &event)
{
  switch (event.type) {
    Window *w;

  case SDL_VIDEOEXPOSE:
    invalidated_lock.Lock();
    invalidated = false;
    invalidated_lock.Unlock();

    expose();
    return true;

  case SDL_KEYDOWN:
    w = get_focused_window();
    if (w == NULL)
      w = this;

    return w->on_key_down(event.key.keysym.sym);

  case SDL_KEYUP:
    w = get_focused_window();
    if (w == NULL)
      w = this;

    return w->on_key_up(event.key.keysym.sym);

  case SDL_MOUSEMOTION:
    // XXX keys
    return on_mouse_move(event.motion.x, event.motion.y, 0);

  case SDL_MOUSEBUTTONDOWN:
    static PeriodClock double_click;
    return double_click.check_always_update(300)
      ? on_mouse_down(event.button.x, event.button.y)
      : on_mouse_double(event.button.x, event.button.y);

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
#endif /* !ENABLE_SDL */

int
TopWindow::event_loop()
{
#ifdef ANDROID
  update();

  EventLoop loop(*event_queue, *this);
  Event event;
  while (loop.get(event))
    loop.dispatch(event);

  return 0;

#elif defined(ENABLE_SDL)
  update();

  EventLoop loop(*this);
  SDL_Event event;
  while (loop.get(event))
    loop.dispatch(event);

  return 0;

#else /* !ENABLE_SDL */

  EventLoop loop;
  MSG msg;
  while (loop.get(msg))
    loop.dispatch(msg);

  return msg.wParam;
#endif /* !ENABLE_SDL */
}
