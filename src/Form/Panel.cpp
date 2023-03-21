// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Panel.hpp"
#include "Look/DialogLook.hpp"

void
PanelControl::Create(ContainerWindow &parent, [[maybe_unused]] const DialogLook &look,
                     const PixelRect &rc,
                     const WindowStyle style)
{
#ifdef HAVE_CLIPPING
  SolidContainerWindow::Create(parent, rc, look.background_color, style);
#else
  ContainerWindow::Create(parent, rc, style);
#endif
}
