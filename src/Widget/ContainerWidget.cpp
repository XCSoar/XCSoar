// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContainerWidget.hpp"
#include "ui/window/ContainerWindow.hpp"

void
ContainerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();

  auto container = std::make_unique<ContainerWindow>();
  container->Create(parent, rc, style);
  SetWindow(std::move(container));
}


