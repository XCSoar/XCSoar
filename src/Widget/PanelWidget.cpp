// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PanelWidget.hpp"
#include "ui/window/Window.hpp"
#include "UIGlobals.hpp"
#include "Form/Panel.hpp"

void
PanelWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.ControlParent();
  style.Hide();

  SetWindow(std::make_unique<PanelControl>(parent,
                                           UIGlobals::GetDialogLook(),
                                           rc,
                                           style));
}
