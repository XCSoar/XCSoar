// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Panel.hpp"

void
PanelControl::Create(ContainerWindow &parent,
                     [[maybe_unused]] const DialogLook &look,
                     const PixelRect &rc,
                     const WindowStyle style)
{
  ContainerWindow::Create(parent, rc, style);
}
