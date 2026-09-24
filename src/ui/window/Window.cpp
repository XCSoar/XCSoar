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

#if defined(_WIN32) && !defined(NDEBUG)
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
#ifdef _WIN32
  assert(GetCurrentThreadId() == OpenGL::thread);
#else
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#endif
#endif
}

void
Window::AssertThreadOrUndefined() const noexcept
{
#ifdef ENABLE_OPENGL
#ifdef _WIN32
  assert(GetCurrentThreadId() == OpenGL::thread);
#else
  assert(pthread_equal(pthread_self(), OpenGL::thread));
#endif
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

  OnDestroy();

  size = {0, 0};
}

ContainerWindow *
Window::GetRootOwner() noexcept
{
  assert(IsDefined());

  if (parent == nullptr)
    /* no parent?  We must be a ContainerWindow instance */
    return (ContainerWindow *)this;

  ContainerWindow *root = parent;
  while (root->parent != nullptr)
    root = root->parent;

  return root;
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
  visible = false;

  if (capture)
    ReleaseCapture();

  if (parent != nullptr) {
    parent->RemoveChild(*this);
    parent = nullptr;
  }
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
Window::OnMultiTouchMove([[maybe_unused]] PixelPoint a,
                         [[maybe_unused]] PixelPoint b) noexcept
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
  ReleaseCapture();
}

void
Window::OnSetFocus() noexcept
{
  assert(!focused);

  focused = true;
}

void
Window::OnKillFocus() noexcept
{
  assert(focused);

  ReleaseCapture();

  focused = false;
}
