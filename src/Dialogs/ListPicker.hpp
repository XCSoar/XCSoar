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

#ifndef XCSOAR_DIALOGS_LIST_PICKER_HPP
#define XCSOAR_DIALOGS_LIST_PICKER_HPP

#include <tchar.h>

class ListItemRenderer;

/** returns string of item's help text **/
typedef const TCHAR* (*ItemHelpCallback_t)(unsigned item);

/**
 * Shows a list dialog and lets the user pick an item.
 * @param caption
 * @param num_items
 * @param initial_value
 * @param item_height
 * @param item_renderer Paint a single item
 * @param update Update per timer
 * @param help_text enable the "Help" button and show this text on click
 * @param itemhelp_callback Callback to return string for current item help
 * @param extra_caption caption of another button that closes the
 * dialog (nullptr disables it)
 * @return the list index, -1 if the user cancelled the dialog, -2 if
 * the user clicked the "extra" button
 */
int
ListPicker(const TCHAR *caption,
           unsigned num_items, unsigned initial_value,
           unsigned item_height,
           ListItemRenderer &item_renderer, bool update = false,
           const TCHAR *help_text = nullptr,
           ItemHelpCallback_t itemhelp_callback = nullptr,
           const TCHAR *extra_caption=nullptr);

#endif
