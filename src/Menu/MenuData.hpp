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
  unsigned event;

  void Clear() {
    label = NULL;
    event = 0;
  }

  constexpr
  bool IsDefined() const {
    return event > 0;
  }

  /**
   * Does this item have a dynamic label?  It may need updates more
   * often, because the variables that the label depends on may change
   * at any time.
   */
  gcc_pure
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
  void Clear();

  const MenuItem &operator[](unsigned i) const {
    return items[i];
  }

  void Add(const TCHAR *label, int location, unsigned event_id);

  gcc_pure
  int FindByEvent(unsigned event) const;
};

#endif
