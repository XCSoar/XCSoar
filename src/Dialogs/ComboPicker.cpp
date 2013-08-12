/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Form/List.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Base.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

static const ComboList *ComboListPopup;

class ComboPickerSupport : public ListItemRenderer {
  const ComboList &combo_list;
  const UPixelScalar padding;

public:
  ComboPickerSupport(const ComboList &_combo_list,
                     const UPixelScalar _padding)
    :combo_list(_combo_list), padding(_padding) {}


  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned i) override {
    canvas.DrawClippedText(rc.left + padding,
                           rc.top + padding, rc,
                           combo_list[i].StringValueFormatted);
  }
};

static const TCHAR*
OnItemHelp(unsigned i)
{
  if ((*ComboListPopup)[i].StringHelp)
    return (*ComboListPopup)[i].StringHelp;

  return _T("");
}

int
ComboPicker(const TCHAR *caption,
            const ComboList &combo_list,
            const TCHAR *help_text,
            bool enable_item_help)
{
  ComboListPopup = &combo_list;

  const UPixelScalar font_height =
    UIGlobals::GetDialogLook().text_font->GetHeight() + Layout::FastScale(2);
  const UPixelScalar max_height = Layout::GetMaximumControlHeight();
  const UPixelScalar row_height = font_height >= max_height
    ? font_height
    /* this formula is supposed to be a compromise between too small
       and too large: */
    : (font_height + max_height) / 2;

  const UPixelScalar padding = (row_height - font_height) / 2;

  ComboPickerSupport support(combo_list, padding);
  return ListPicker(caption,
                    combo_list.size(),
                    combo_list.ComboPopupItemSavedIndex,
                    row_height,
                    support, false,
                    help_text,
                    enable_item_help ? OnItemHelp : NULL);
}

bool
ComboPicker(const TCHAR *caption, DataField &df,
            const TCHAR *help_text)
{
  StaticString<256> buffer;
  const TCHAR *reference = nullptr;

  while (true) {
    const ComboList combo_list = df.CreateComboList(reference);
    ComboListPopup = &combo_list;

    int idx = ComboPicker(caption, combo_list, help_text,
                          df.GetItemHelpEnabled());
    if (idx < 0)
      return false;

    const ComboList::Item &item = combo_list[idx];

    // OK/Select
    if (item.DataFieldIndex == ComboList::Item::NEXT_PAGE) {
      // we're last in list and the want more past end of list so select last real list item and reopen
      // we'll reopen, so don't call xcsoar data changed routine yet
      reference = buffer = combo_list[idx - 1].StringValue;
    } else if (item.DataFieldIndex == ComboList::Item::PREVIOUS_PAGE) {
      // same as above but lower items needed
      reference = buffer = combo_list[idx + 1].StringValue;
    } else {
      df.SetFromCombo(item.DataFieldIndex, item.StringValue);
      return true;
    }
  } // loop reopen combo if <<More>>  or <<Less>> picked
}
