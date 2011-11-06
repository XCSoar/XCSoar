/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_MENU_DATA_HPP
#define XCSOAR_MENU_DATA_HPP

#include "Compiler.h"

#include <tchar.h>
#include <string.h>

/**
 * Data of an item in the mode menu.
 */
class MenuItem {
public:
  const TCHAR *label;
  int event;

  gcc_constexpr_ctor
  MenuItem():label(NULL), event(-1) {}

  gcc_constexpr_method
  bool defined() const {
    return event > 0;
  }

  /**
   * Does this item have a dynamic label?  It may need updates more
   * often, because the variables that the label depends on may change
   * at any time.
   */
  bool IsDynamic() const {
    return label != NULL && _tcsstr(label, _T("$(")) != NULL;
  }
};

/**
 * A container for MenuItem objects.
 */
class Menu {
public:
  enum {
    MAX_ITEMS = 32,
  };

protected:
  MenuItem items[MAX_ITEMS];

public:
  const MenuItem &operator[](unsigned i) const {
    return items[i];
  }

  void Add(const TCHAR *label, int location, int event_id);

  int FindByEvent(int event) const;
};

#endif
