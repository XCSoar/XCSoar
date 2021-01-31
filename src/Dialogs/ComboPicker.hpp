/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_DIALOGS_COMBO_PICKER_HPP
#define XCSOAR_DIALOGS_COMBO_PICKER_HPP

#include <tchar.h>

class ComboList;
class DataField;

int
ComboPicker(const TCHAR *caption,
            const ComboList &combo_list,
            const TCHAR *help_text = nullptr,
            bool enable_item_help = false,
            const TCHAR *extra_caption=nullptr);

/**
 * @return true if the user has selected a new value (though it may be
 * equal to the old one)
 */
bool
ComboPicker(const TCHAR *caption, DataField &df,
            const TCHAR *help_text = nullptr);

#endif
