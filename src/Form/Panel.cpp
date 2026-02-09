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
  SetGradientTopColor(look.background_gradient_top_color);
#else
  /* on non-clipping platforms (OpenGL), the parent's gradient is
     already in the framebuffer; staying transparent avoids drawing
     a second gradient that would visually restart below the content */
  ContainerWindow::Create(parent, rc, style);
#endif
}
