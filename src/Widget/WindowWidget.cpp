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

#include "WindowWidget.hpp"
#include "Screen/Window.hpp"

WindowWidget::WindowWidget(Window *_window)
  :window(_window) {
  assert(window != nullptr);
  assert(!window->IsDefined() || !window->IsVisible());
}

void
WindowWidget::DeleteWindow()
{
  assert(window != nullptr);

  /* we must call Window::Destroy() explicitly here, because when
     Window::~Window() attempts to do that, it's too late already to
     invoke virtual overrides */
  window->Destroy();
  delete window;
}

void
WindowWidget::Show(const PixelRect &rc)
{
  assert(window != nullptr);
  assert(window->IsDefined());
  assert(!window->IsVisible());

  window->MoveAndShow(rc);
}

void
WindowWidget::Hide()
{
  assert(window != nullptr);
  assert(window->IsDefined());
  assert(window->IsVisible());

  window->FastHide();
}

void
WindowWidget::Move(const PixelRect &rc)
{
  assert(window != nullptr);
  assert(window->IsDefined());
  assert(window->IsVisible());

  window->Move(rc);
}
