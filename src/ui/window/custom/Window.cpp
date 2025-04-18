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

  return parent != nullptr &&
    GetSize().width >= parent->GetSize().width &&
    GetSize().height >= parent->GetSize().height;
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
