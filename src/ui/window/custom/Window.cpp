// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../Window.hpp"
#include "../ContainerWindow.hpp"
#include "Screen/Debug.hpp"
#include "ui/canvas/Canvas.hpp"

void
Window::Create(ContainerWindow *parent, PixelRect rc,
               const WindowStyle window_style) noexcept
{
  assert(IsScreenInitialized());
  assert(rc.left <= rc.right);
  assert(rc.GetWidth() < 0x8000);
  assert(rc.top <= rc.bottom);
  assert(rc.GetHeight() < 0x8000);

  this->parent = parent;
  position = rc.GetOrigin();
  size = rc.GetSize();

  tab_stop = window_style.tab_stop;
  control_parent = window_style.control_parent;
  visible = window_style.visible;
  enabled = window_style.enabled;
  has_border = window_style.has_border;

  if (parent != nullptr)
    parent->AddChild(*this);

  OnCreate();
  OnResize(size);
}

PixelRect
Window::GetParentClientRect() const noexcept
{
  assert(IsDefined());
  assert(parent != nullptr);

  return parent->GetClientRect();
}

bool
Window::IsMaximised() const noexcept
{
  assert(IsDefined());

  if (parent == nullptr)
    return false;

  /* compare with the parent's client rect, not with its raw size: a
     #TopWindow may reserve space at the edges (e.g. the iOS safe
     area), and a window which fills that client area is "maximised",
     even though it is smaller than the parent window itself.  For all
     other windows, GetClientRect() is PixelRect{GetSize()}, so this is
     equivalent to comparing the sizes. */
  const PixelSize parent_size = parent->GetClientRect().GetSize();

  return GetSize().width >= parent_size.width &&
    GetSize().height >= parent_size.height;
}

void
Window::SetEnabled(bool enabled) noexcept
{
  AssertThread();

  if (enabled == this->enabled)
    return;

  if (!enabled)
    /* cancel dragging before disabling this Window */
    OnCancelMode();

  this->enabled = enabled;
  Invalidate();
}

Window *
Window::GetFocusedWindow() noexcept
{
  assert(IsDefined());

  return focused ? this : nullptr;
}

void
Window::SetFocus() noexcept
{
  assert(IsDefined());

  if (!IsEnabled())
    return;

  if (parent != nullptr)
    parent->SetActiveChild(*this);

  if (focused)
    return;

  OnSetFocus();
}

void
Window::ClearFocus() noexcept
{
  if (focused) {
    OnKillFocus();

    assert(!focused);
  }
}

void
Window::FocusParent() noexcept
{
  AssertThread();
  assert(parent != nullptr);

  parent->SetFocus();
}

void
Window::SetCapture() noexcept
{
  AssertThread();

  if (parent != nullptr)
    parent->SetChildCapture(this);
  else
    EnableCapture();

  capture = true;
}

void
Window::ReleaseCapture() noexcept
{
  AssertThread();

  capture = false;

  if (parent != nullptr)
    parent->ReleaseChildCapture(this);
  else
    DisableCapture();
}

void
Window::ClearCapture() noexcept
{
  capture = false;
}

void
Window::Invalidate() noexcept
{
  AssertThread();
  assert(IsDefined());

  if (visible && parent != nullptr)
    parent->InvalidateChild(*this);
}

void
Window::Show() noexcept
{
  AssertThread();

  if (visible)
    return;

  visible = true;
  parent->Invalidate();
}

void
Window::Hide() noexcept
{
  AssertThread();

  if (!visible)
    return;

  visible = false;

  /* a window that is no longer on screen must not keep the mouse
     capture: every further event would be routed to it instead of to
     what the user can actually see.  Both calls are cheap no-ops
     unless this window, or a descendant, is capturing. */
  parent->ReleaseChildCapture(this);
  ClearCapture();

  parent->Invalidate();
}

void
Window::BringToTop() noexcept
{
  AssertThread();

  parent->BringChildToTop(*this);
}

void
Window::BringToBottom() noexcept
{
  AssertThread();

  parent->BringChildToBottom(*this);
}
