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

#include "Screen/Window.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Screen/Debug.hpp"

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Event.hpp"
#endif /* ENABLE_SDL */

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Debug.hpp"
#endif

#include <assert.h>

Window::~Window()
{
  reset();
}

#ifndef NDEBUG

void
Window::assert_thread() const
{
#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#elif defined(USE_GDI)
  assert(hWnd != NULL);
  assert(!::IsWindow(hWnd) ||
         ::GetWindowThreadProcessId(hWnd, NULL) == ::GetCurrentThreadId());
#endif
}

void
Window::AssertThreadOrUndefined() const
{
#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#elif defined(USE_GDI)
  assert(hWnd == NULL || !::IsWindow(hWnd) ||
         ::GetWindowThreadProcessId(hWnd, NULL) == ::GetCurrentThreadId());
#endif
}

#endif /* !NDEBUG */

void
Window::reset()
{
  if (!defined())
    return;

  assert(IsScreenInitialized());
  assert_thread();

#ifndef USE_GDI
  on_destroy();

  width = 0;
  height = 0;
#else /* USE_GDI */
  ::DestroyWindow(hWnd);

  /* the on_destroy() method must have cleared the variable by
     now */
  assert(prev_wndproc == NULL || hWnd == NULL);

  hWnd = NULL;
  prev_wndproc = NULL;
#endif /* USE_GDI */
}

ContainerWindow *
Window::get_root_owner()
{
#ifndef USE_GDI
  if (parent == NULL)
    /* no parent?  We must be a ContainerWindow instance */
    return (ContainerWindow *)this;

  ContainerWindow *root = parent;
  while (root->parent != NULL)
    root = root->parent;

  return root;
#else /* USE_GDI */
#ifndef _WIN32_WCE
  HWND hRoot = ::GetAncestor(hWnd, GA_ROOTOWNER);
  if (hRoot == NULL)
    return NULL;
#else
  HWND hRoot = hWnd;
  while (true) {
    HWND hParent = ::GetParent(hRoot);
    if (hParent == NULL)
      break;
    hRoot = hParent;
  }
#endif

  /* can't use the "checked" method get() because hRoot may be a
     dialog, and uses Dialog::DlgProc() */
  return (ContainerWindow *)get_unchecked(hRoot);
#endif /* USE_GDI */
}

bool
Window::on_create()
{
  return true;
}

bool
Window::on_destroy()
{
#ifndef USE_GDI
  if (capture)
    release_capture();

  if (parent != NULL) {
    parent->remove_child(*this);
    parent = NULL;
  }

#ifdef ANDROID
  event_queue->purge(*this);
#else
  EventQueue::purge(*this);
#endif
#else /* USE_GDI */
  assert(hWnd != NULL);

  hWnd = NULL;
#endif /* USE_GDI */

  return true;
}

bool
Window::on_close()
{
  return false;
}

bool
Window::on_resize(unsigned width, unsigned height)
{
  return false;
}

bool
Window::on_mouse_move(int x, int y, unsigned keys)
{
  /* not handled here */
  return false;
}

bool
Window::on_mouse_down(int x, int y)
{
  return false;
}

bool
Window::on_mouse_up(int x, int y)
{
  return false;
}

bool
Window::on_mouse_double(int x, int y)
{
#ifndef USE_GDI
  if (!double_clicks)
    return on_mouse_down(x, y);
#endif

  return false;
}

bool
Window::on_mouse_wheel(int x, int y, int delta)
{
  return false;
}

bool
Window::on_key_check(unsigned key_code) const
{
  return false;
}

bool
Window::on_key_down(unsigned key_code)
{
  return false;
}

bool
Window::on_key_up(unsigned key_code)
{
  return false;
}

bool
Window::on_command(unsigned id, unsigned code)
{
  return false;
}

bool
Window::on_cancel_mode()
{
#ifndef USE_GDI
  release_capture();
#endif

  return false;
}

bool
Window::on_setfocus()
{
#ifndef USE_GDI
  assert(!focused);

  focused = true;
  return true;
#else /* USE_GDI */
  return false;
#endif /* USE_GDI */
}

bool
Window::on_killfocus()
{
#ifndef USE_GDI
  assert(focused);

  release_capture();

  focused = false;
  return true;
#else /* USE_GDI */
  return false;
#endif /* USE_GDI */
}

bool
Window::on_timer(timer_t id)
{
  return false;
}

bool
Window::on_user(unsigned id)
{
  return false;
}

void
Window::on_paint(Canvas &canvas)
{
}

void
Window::on_paint(Canvas &canvas, const PixelRect &dirty)
{
  on_paint(canvas);
}
