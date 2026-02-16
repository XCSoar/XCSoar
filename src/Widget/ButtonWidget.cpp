// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonWidget.hpp"
#include "Form/Button.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "Screen/Layout.hpp"

ButtonWidget::ButtonWidget(std::unique_ptr<ButtonRenderer> _renderer,
                           std::function<void()> _callback) noexcept
  :renderer(std::move(_renderer)),
   callback(std::move(_callback)) {}

ButtonWidget::ButtonWidget(const ButtonLook &look, const char *caption,
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
