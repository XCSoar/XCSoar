// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MultiSelectListWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/Rect.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/Layout.hpp"
#include "util/Macros.hpp"
#include "Asset.hpp"
#include "Form/CheckBox.hpp"

bool
MultiSelectListWidget::KeyPress(unsigned key_code) noexcept
{
  if (ListWidget::KeyPress(key_code))
    return true;

  if (!IsDefined() || !GetList().HasFocus())
    return false;

  switch (key_code) {
  case KEY_RETURN:
  case KEY_SPACE:
    /* Activate (toggle) via ListControl; always consume so the dialog
       action bar does not treat Return as OK while browsing files. */
    GetList().OnKeyFromWidgetParent(KEY_RETURN);
    return true;

  default:
    return false;
  }
}

void
MultiSelectListWidget::DrawCheckboxText(Canvas &canvas, const PixelRect &rc,
                                        const char *text, bool checked) noexcept
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
               checked, focused, false, true);

  // Draw text to the right of the checkbox, clipped to row
  const Font &font = *look.list.font;
  const PixelPoint text_pos(box_rc.right + 2 * (int)padding,
                           rc.top + (int)((rc.GetHeight() - font.GetHeight()) / 2));

  canvas.Select(font);
  /* ListControl already set this row's text colour: white on the
     highlight in both themes.  The plain colour is black in the
     bright theme and disappears on that bar. */
  canvas.SetBackgroundTransparent();
  canvas.DrawClippedText(text_pos, rc, text);
}
