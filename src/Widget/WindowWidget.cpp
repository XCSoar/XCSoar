/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
