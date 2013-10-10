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

#ifndef XCSOAR_FORM_TAB_MENU_DATA_HPP
#define XCSOAR_FORM_TAB_MENU_DATA_HPP

#include <tchar.h>

class Widget;

struct TabMenuGroup {
  const TCHAR *caption;
};

/**
 * List of all submenu items in array of MenuPageDescription[0 to
 * (n-1)].  The menus must be sorted by main_menu_index and the order
 * to appear.
 */
struct TabMenuPage {
  const TCHAR *menu_caption;

  /* The main menu page Enter menu page into the array
   * 0 to (GetNumMainMenuCaptions() - 1) */
  unsigned main_menu_index;

  Widget *(*Load)();
};

#endif
