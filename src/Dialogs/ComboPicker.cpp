/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Form/DataField/Base.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Util/StaticString.hxx"

static const ComboList *ComboListPopup;

class ComboPickerSupport : public ListItemRenderer {
  const ComboList &combo_list;
  TextRowRenderer row_renderer;

public:
  ComboPickerSupport(const ComboList &_combo_list)
    :combo_list(_combo_list) {}

  unsigned CalculateLayout(const DialogLook &look) {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned i) override {
    row_renderer.DrawTextRow(canvas, rc, combo_list[i].display_string.c_str());
  }
};

static const TCHAR*
OnItemHelp(unsigned i)
{
  if (!(*ComboListPopup)[i].help_text.IsNull())
    return (*ComboListPopup)[i].help_text.c_str();

  return _T("");
}

int
ComboPicker(const TCHAR *caption,
            const ComboList &combo_list,
            const TCHAR *help_text,
            bool enable_item_help,
            const TCHAR *extra_caption)
{
  ComboListPopup = &combo_list;

  ComboPickerSupport support(combo_list);
  return ListPicker(caption,
                    combo_list.size(),
                    combo_list.current_index,
                    support.CalculateLayout(UIGlobals::GetDialogLook()),
                    support, false,
                    help_text,
                    enable_item_help ? OnItemHelp : nullptr,
                    extra_caption);
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
    if (item.int_value == ComboList::Item::NEXT_PAGE) {
      // we're last in list and the want more past end of list so select last real list item and reopen
      // we'll reopen, so don't call xcsoar data changed routine yet
      reference = buffer = combo_list[idx - 1].string_value.c_str();
    } else if (item.int_value == ComboList::Item::PREVIOUS_PAGE) {
      // same as above but lower items needed
      reference = buffer = combo_list[idx + 1].string_value.c_str();
    } else {
      df.SetFromCombo(item.int_value, item.string_value.c_str());
      return true;
    }
  } // loop reopen combo if <<More>>  or <<Less>> picked
}
