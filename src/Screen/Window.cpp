/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Debug.hpp"
#endif

#include <assert.h>

Window::~Window()
{
  Destroy();
}

#ifndef NDEBUG

void
Window::AssertThread() const
{
  assert(IsDefined());

#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#elif defined(USE_GDI)
  assert(hWnd != nullptr);
  assert(!::IsWindow(hWnd) ||
         ::GetWindowThreadProcessId(hWnd, nullptr) == ::GetCurrentThreadId());
#endif
}

void
Window::AssertThreadOrUndefined() const
{
#ifdef ENABLE_OPENGL
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#elif defined(USE_GDI)
  assert(hWnd == nullptr || !::IsWindow(hWnd) ||
         ::GetWindowThreadProcessId(hWnd, nullptr) == ::GetCurrentThreadId());
#endif
}

#endif /* !NDEBUG */

void
Window::Destroy()
{
  if (!IsDefined())
    return;

  assert(IsScreenInitialized());
  AssertThread();

#ifndef USE_GDI
  OnDestroy();

  size = {0, 0};
#else /* USE_GDI */
  ::DestroyWindow(hWnd);

  /* the OnDestroy() method must have cleared the variable by
     now */
  assert(prev_wndproc == nullptr || hWnd == nullptr);

  hWnd = nullptr;
  prev_wndproc = nullptr;
#endif /* USE_GDI */
}

ContainerWindow *
Window::GetRootOwner()
{
  assert(IsDefined());

#ifndef USE_GDI
  if (parent == nullptr)
    /* no parent?  We must be a ContainerWindow instance */
    return (ContainerWindow *)this;

  ContainerWindow *root = parent;
  while (root->parent != nullptr)
    root = root->parent;

  return root;
#else /* USE_GDI */
#ifndef _WIN32_WCE
  HWND hRoot = ::GetAncestor(hWnd, GA_ROOTOWNER);
  if (hRoot == nullptr)
    return nullptr;
#else
  HWND hRoot = hWnd;
  while (true) {
    HWND hParent = ::GetParent(hRoot);
    if (hParent == nullptr)
      break;
    hRoot = hParent;
  }
#endif

  /* can't use the "checked" method get() because hRoot may be a
     dialog, and uses Dialog::DlgProc() */
  return (ContainerWindow *)GetUnchecked(hRoot);
#endif /* USE_GDI */
}

void
Window::OnCreate()
{
}

void
Window::OnDestroy()
{
#ifndef USE_GDI
  visible = false;

  if (capture)
    ReleaseCapture();

  if (parent != nullptr) {
    parent->RemoveChild(*this);
    parent = nullptr;
  }

  event_queue->Purge(*this);
#else /* USE_GDI */
  assert(hWnd != nullptr);

  hWnd = nullptr;
#endif /* USE_GDI */
}

void
Window::OnResize(PixelSize new_size)
{
}

bool
Window::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  /* not handled here */
  return false;
}

bool
Window::OnMouseDown(PixelScalar x, PixelScalar y)
{
  return false;
}

bool
Window::OnMouseUp(PixelScalar x, PixelScalar y)
{
  return false;
}

bool
Window::OnMouseDouble(PixelScalar x, PixelScalar y)
{
#ifndef USE_GDI
  if (!double_clicks)
    return OnMouseDown(x, y);
#endif

  return false;
}

bool
Window::OnMouseWheel(PixelScalar x, PixelScalar y, int delta)
{
  return false;
}

#ifdef HAVE_MULTI_TOUCH

bool
Window::OnMultiTouchDown()
{
  return false;
}

bool
Window::OnMultiTouchUp()
{
  return false;
}

#endif /* HAVE_MULTI_TOUCH */

bool
Window::OnKeyCheck(unsigned key_code) const
{
  return false;
}

bool
Window::OnKeyDown(unsigned key_code)
{
  return false;
}

bool
Window::OnKeyUp(unsigned key_code)
{
  return false;
}

bool
Window::OnCharacter(unsigned ch)
{
  return false;
}

bool
Window::OnCommand(unsigned id, unsigned code)
{
  return false;
}

void
Window::OnCancelMode()
{
#ifndef USE_GDI
  ReleaseCapture();
#endif
}

void
Window::OnSetFocus()
{
#ifndef USE_GDI
  assert(!focused);

  focused = true;
#endif /* USE_GDI */
}

void
Window::OnKillFocus()
{
#ifndef USE_GDI
  assert(focused);

  ReleaseCapture();

  focused = false;
#endif /* USE_GDI */
}

bool
Window::OnTimer(WindowTimer &timer)
{
  return false;
}

bool
Window::OnUser(unsigned id)
{
  return false;
}

void
Window::OnPaint(Canvas &canvas)
{
}

void
Window::OnPaint(Canvas &canvas, const PixelRect &dirty)
{
  OnPaint(canvas);
}
