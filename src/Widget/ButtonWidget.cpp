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

#include "ButtonWidget.hpp"
#include "Form/Button.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Screen/Layout.hpp"

ButtonWidget::ButtonWidget(std::unique_ptr<ButtonRenderer> _renderer,
                           std::function<void()> _callback) noexcept
  :renderer(std::move(_renderer)),
   callback(std::move(_callback)) {}

ButtonWidget::ButtonWidget(const ButtonLook &look, const TCHAR *caption,
                           std::function<void()> _callback) noexcept
  :renderer(std::make_unique<TextButtonRenderer>(look, caption)),
   callback(std::move(_callback)) {}

ButtonWidget::~ButtonWidget() noexcept = default;

ButtonRenderer &
ButtonWidget::GetRenderer() noexcept
{
  return IsDefined()
    ? ((Button &)GetWindow()).GetRenderer()
    : *renderer;
}

const ButtonRenderer &
ButtonWidget::GetRenderer() const noexcept
{
  return IsDefined()
    ? ((const Button &)GetWindow()).GetRenderer()
    : *renderer;
}

void
ButtonWidget::Invalidate() noexcept
{
  assert(IsDefined());

  ((Button &)GetWindow()).Invalidate();
}

PixelSize
ButtonWidget::GetMinimumSize() const noexcept
{
  return PixelSize(GetRenderer().GetMinimumButtonWidth(),
                   Layout::GetMinimumControlHeight());
}

PixelSize
ButtonWidget::GetMaximumSize() const noexcept
{
  return PixelSize(GetRenderer().GetMinimumButtonWidth() + Layout::GetMaximumControlHeight(),
                   Layout::GetMaximumControlHeight());
}

void
ButtonWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  SetWindow(std::make_unique<Button>(parent, rc, style, std::move(renderer),
                                     std::move(callback)));
}

bool
ButtonWidget::SetFocus() noexcept
{
  GetWindow().SetFocus();
  return true;
}
