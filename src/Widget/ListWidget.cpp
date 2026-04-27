// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ListWidget.hpp"
#include "ui/window/Window.hpp"
#include "ui/event/KeyCode.hpp"
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

bool
ListWidget::KeyPress(unsigned key_code) noexcept
{
  if (key_code != KEY_UP && key_code != KEY_DOWN)
    return false;

  /* Only when the list has focus: same client area as filter rows, etc. in
     e.g. `ShowWaypointListDialog` — if we always returned true, Up/Down
     would never reach the filter (file/enum) or other focused controls. */
  if (!IsDefined() || !GetList().HasFocus())
    return false;

  /* Route in #WidgetDialog::OnAnyKeyDown *before* #WndForm maps these
     keys to tab/FocusNext, and before #ListControl::OnKeyCheck is used
     to decide.  When #OnKeyDown is false (e.g. at list edge), return
     false so focus can move to the next/previous control (e.g. action
     bar or filter).  Left/right: #ButtonPanel::KeyPress. */
  return GetList().OnKeyFromWidgetParent(key_code);
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
