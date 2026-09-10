// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BorderWindow.hpp"
#include "Border.hpp"
#include "Look/InfoBoxLook.hpp"

InfoBoxBorderWindow::InfoBoxBorderWindow(ContainerWindow &parent, PixelRect rc,
                                         unsigned _border_kind,
                                         const InfoBoxLook &_look) noexcept
  :look(_look), border_kind(_border_kind)
{
  WindowStyle style;
  style.Hide();

  /* the slot shows the map, so the map window below shall receive all
     input events for this area */
  style.Disable();

  Create(parent, rc, style);

#ifndef USE_WINUSER
  /* only the border lines are painted; the map window below stays
     visible */
  SetTransparent();
#endif
}

void
InfoBoxBorderWindow::SetBorderKind(unsigned _border_kind) noexcept
{
  if (_border_kind == border_kind)
    return;

  border_kind = _border_kind;
  Invalidate();
}

void
InfoBoxBorderWindow::OnPaint(Canvas &canvas) noexcept
{
  DrawBorder(canvas, border_kind, look.border_pen);
}
