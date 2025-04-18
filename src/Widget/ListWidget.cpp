// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ListWidget.hpp"
#include "ui/window/Window.hpp"
#include "Screen/Layout.hpp"

PixelSize
ListWidget::GetMinimumSize() const noexcept
{
  return { unsigned(Layout::Scale(200u)),
      /* a list makes only sense when the user sees more than one row
         at a time */
      2u * GetList().GetItemHeight() };
}

PixelSize
ListWidget::GetMaximumSize() const noexcept
{
  return PixelSize { 4096, 4096 };
}

ListControl &
ListWidget::CreateList(ContainerWindow &parent, const DialogLook &look,
                       const PixelRect &rc, unsigned row_height) noexcept
{
  WindowStyle list_style;
  list_style.Hide();
  list_style.TabStop();
  list_style.Border();

  auto list = std::make_unique<ListControl>(parent, look, rc,
                                            list_style, row_height);
  list->SetItemRenderer(this);
  list->SetCursorHandler(this);
  SetWindow(std::move(list));
  return GetList();
}
