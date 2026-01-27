// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MultiSelectListWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/Rect.hpp"
#include "Screen/Layout.hpp"
#include "util/Macros.hpp"
#include "Asset.hpp"
#include "Form/CheckBox.hpp"

void
MultiSelectListWidget::DrawCheckboxText(Canvas &canvas, const PixelRect &rc,
                                        const TCHAR *text, bool selected) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const bool focused = !HasCursorKeys() || GetList().HasFocus();

  const unsigned padding = Layout::GetTextPadding();
  unsigned box_size = rc.GetHeight() > 2 * padding ? rc.GetHeight() - 2 * padding : 0;
  // Draw checkbox box at left of row
  PixelRect box_rc;
  box_rc.left = rc.left + (int)padding;
  box_rc.top = rc.top + (int)padding;
  box_rc.right = box_rc.left + (int)box_size;
  box_rc.bottom = box_rc.top + (int)box_size;

  DrawCheckBox(canvas, UIGlobals::GetDialogLook(), box_rc,
               selected, focused, false, true);

  // Draw text to the right of the checkbox, clipped to row
  const Font &font = *look.list.font;
  const PixelPoint text_pos(box_rc.right + 2 * (int)padding,
                           rc.top + (int)((rc.GetHeight() - font.GetHeight()) / 2));

  canvas.Select(font);
  canvas.SetTextColor(look.list.GetTextColor(false, focused, false));
  canvas.SetBackgroundTransparent();
  canvas.DrawClippedText(text_pos, rc, text);
}
