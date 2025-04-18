// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Window.hpp"
#include "ContainerWindow.hpp"
#include "Screen/Debug.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Debug.hpp"
#endif

#include <cassert>

#if defined(USE_WINUSER) && !defined(NDEBUG)
#include <processthreadsapi.h>
#endif

Window::~Window() noexcept
{
  Destroy();
}

#ifndef NDEBUG

void
Window::AssertThread() const noexcept
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
Window::AssertThreadOrUndefined() const noexcept
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
Window::Destroy() noexcept
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
Window::GetRootOwner() noexcept
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
Window::ScrollParentTo() noexcept
{
  if (auto *parent = GetParent())
    parent->ScrollTo(GetPosition());
}

void
Window::OnCreate()
{
}

void
Window::OnDestroy() noexcept
{
#ifndef USE_WINUSER
  visible = false;

  if (capture)
    ReleaseCapture();

  if (parent != nullptr) {
    parent->RemoveChild(*this);
    parent = nullptr;
  }
#else /* USE_WINUSER */
  assert(hWnd != nullptr);

  hWnd = nullptr;
#endif /* USE_WINUSER */
}

void
Window::OnResize([[maybe_unused]] PixelSize new_size) noexcept
{
}

bool
Window::OnMouseMove([[maybe_unused]] PixelPoint p,
                    [[maybe_unused]] unsigned keys) noexcept
{
  /* not handled here */
  return false;
}

bool
Window::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  return false;
}

bool
Window::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  return false;
}

bool
Window::OnMouseDouble(PixelPoint p) noexcept
{
  /* fall back to OnMouseDown() if the class didn't override
     OnMouseDouble() */
  return OnMouseDown(p);
}

bool
Window::OnMouseWheel([[maybe_unused]] PixelPoint p,
                     [[maybe_unused]] int delta) noexcept
{
  return false;
}

#ifdef HAVE_MULTI_TOUCH

bool
Window::OnMultiTouchDown() noexcept
{
  return false;
}

bool
Window::OnMultiTouchUp() noexcept
{
  return false;
}

#endif /* HAVE_MULTI_TOUCH */

bool
Window::OnKeyCheck([[maybe_unused]] unsigned key_code) const noexcept
{
  return false;
}

bool
Window::OnKeyDown([[maybe_unused]] unsigned key_code) noexcept
{
  return false;
}

bool
Window::OnKeyUp([[maybe_unused]] unsigned key_code) noexcept
{
  return false;
}

bool
Window::OnCharacter([[maybe_unused]] unsigned ch) noexcept
{
  return false;
}

void
Window::OnCancelMode() noexcept
{
#ifndef USE_WINUSER
  ReleaseCapture();
#endif
}

void
Window::OnSetFocus() noexcept
{
#ifndef USE_WINUSER
  assert(!focused);

  focused = true;
#endif /* USE_WINUSER */
}

void
Window::OnKillFocus() noexcept
{
#ifndef USE_WINUSER
  assert(focused);

  ReleaseCapture();

  focused = false;
#endif /* USE_WINUSER */
}
