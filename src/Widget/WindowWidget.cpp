// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowWidget.hpp"
#include "ui/window/Window.hpp"

WindowWidget::WindowWidget() noexcept = default;

WindowWidget::WindowWidget(std::unique_ptr<Window> _window) noexcept
  :window(std::move(_window))
{
  assert(window != nullptr);
  assert(!window->IsDefined() || !window->IsVisible());
}

WindowWidget::~WindowWidget() noexcept
{
  /* we must call Window::Destroy() explicitly here, because when
     Window::~Window() attempts to do that, it's too late already to
     invoke virtual overrides */
  if (window)
    window->Destroy();
}

void
WindowWidget::SetWindow(std::unique_ptr<Window> &&_window) noexcept
{
  assert(window == nullptr);
  assert(_window != nullptr);

  window = std::move(_window);
}

void
WindowWidget::DeleteWindow() noexcept
{
  assert(window != nullptr);

  /* we must call Window::Destroy() explicitly here, because when
     Window::~Window() attempts to do that, it's too late already to
     invoke virtual overrides */
  window->Destroy();
  window.reset();
}

void
WindowWidget::Show(const PixelRect &rc) noexcept
{
  assert(window != nullptr);
  assert(window->IsDefined());
  assert(!window->IsVisible());

  window->MoveAndShow(rc);
}

void
WindowWidget::Hide() noexcept
{
  assert(window != nullptr);
  assert(window->IsDefined());
  assert(window->IsVisible());

  window->FastHide();
}

void
WindowWidget::Move(const PixelRect &rc) noexcept
{
  assert(window != nullptr);
  assert(window->IsDefined());
  assert(window->IsVisible());

  window->Move(rc);
}

bool
WindowWidget::HasFocus() const noexcept
{
  assert(window != nullptr);
  assert(window->IsDefined());

  return window->HasFocus();
}
