// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextListWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

void
TextListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  ListWidget::Prepare(parent, rc);

  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font));
}

void
TextListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                            unsigned i) noexcept
{
  auto text = GetRowText(i);
  if (text != nullptr)
    row_renderer.DrawTextRow(canvas, rc, text);
}
