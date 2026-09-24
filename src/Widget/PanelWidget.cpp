// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PanelWidget.hpp"
#include "ui/window/Window.hpp"
#include "Form/Panel.hpp"

void
PanelWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.ControlParent();
  style.Hide();

  SetWindow(std::make_unique<PanelControl>(parent, rc, style));
}
