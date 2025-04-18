// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DrawWidget.hpp"
#include "Form/Draw.hpp"

PixelSize
DrawWidget::GetMinimumSize() const noexcept
{
  return {};
}

PixelSize
DrawWidget::GetMaximumSize() const noexcept
{
  return {};
}

void
DrawWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();

  auto w = std::make_unique<WndOwnerDrawFrame>();
  w->Create(parent, rc, style, std::move(draw_function));

  SetWindow(std::move(w));
}
