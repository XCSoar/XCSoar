/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#elif defined(USE_WINUSER)
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
#elif defined(USE_WINUSER)
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

#ifndef USE_WINUSER
  OnDestroy();

  size = {0, 0};
#else /* USE_WINUSER */
  ::DestroyWindow(hWnd);
  hWnd = nullptr;
#endif /* USE_WINUSER */
}

ContainerWindow *
Window::GetRootOwner()
{
  assert(IsDefined());

#ifndef USE_WINUSER
  if (parent == nullptr)
    /* no parent?  We must be a ContainerWindow instance */
    return (ContainerWindow *)this;

  ContainerWindow *root = parent;
  while (root->parent != nullptr)
    root = root->parent;

  return root;
#else /* USE_WINUSER */
  HWND hRoot = ::GetAncestor(hWnd, GA_ROOTOWNER);
  if (hRoot == nullptr)
    return nullptr;

  /* can't use the "checked" method get() because hRoot may be a
     dialog, and uses Dialog::DlgProc() */
  return (ContainerWindow *)GetUnchecked(hRoot);
#endif /* USE_WINUSER */
}

void
Window::OnCreate()
{
}

void
Window::OnDestroy()
{
#ifndef USE_WINUSER
  visible = false;

  if (capture)
    ReleaseCapture();

  if (parent != nullptr) {
    parent->RemoveChild(*this);
    parent = nullptr;
  }

  event_queue->Purge(*this);
#else /* USE_WINUSER */
  assert(hWnd != nullptr);

  hWnd = nullptr;
#endif /* USE_WINUSER */
}

void
Window::OnResize(PixelSize new_size)
{
}

bool
Window::OnMouseMove(PixelPoint p, unsigned keys)
{
  /* not handled here */
  return false;
}

bool
Window::OnMouseDown(PixelPoint p)
{
  return false;
}

bool
Window::OnMouseUp(PixelPoint p)
{
  return false;
}

bool
Window::OnMouseDouble(PixelPoint p)
{
  /* fall back to OnMouseDown() if the class didn't override
     OnMouseDouble() */
  return OnMouseDown(p);
}

bool
Window::OnMouseWheel(PixelPoint p, int delta)
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

void
Window::OnCancelMode()
{
#ifndef USE_WINUSER
  ReleaseCapture();
#endif
}

void
Window::OnSetFocus()
{
#ifndef USE_WINUSER
  assert(!focused);

  focused = true;
#endif /* USE_WINUSER */
}

void
Window::OnKillFocus()
{
#ifndef USE_WINUSER
  assert(focused);

  ReleaseCapture();

  focused = false;
#endif /* USE_WINUSER */
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
