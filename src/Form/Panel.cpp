// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Panel.hpp"
#include "Look/DialogLook.hpp"

void
PanelControl::Create(ContainerWindow &parent, [[maybe_unused]] const DialogLook &look,
                     const PixelRect &rc,
                     const WindowStyle style)
{
#if defined(HAVE_CLIPPING) || defined(ENABLE_OPENGL)
  SolidContainerWindow::Create(parent, rc, look.background_color, style);
  SetGradientTopColor(look.background_gradient_top_color);
#else
  ContainerWindow::Create(parent, rc, style);
#endif
}
