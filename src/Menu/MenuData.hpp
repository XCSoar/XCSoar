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

#ifndef XCSOAR_MENU_DATA_HPP
#define XCSOAR_MENU_DATA_HPP

#include <tchar.h>
#include <string.h>

#include <array>

/**
 * Data of an item in the mode menu.
 */
class MenuItem {
public:
  const TCHAR *label;
  unsigned event;

  void Clear() noexcept {
    label = nullptr;
    event = 0;
  }

  constexpr bool IsDefined() const noexcept {
    return event > 0;
  }

  /**
   * Does this item have a dynamic label?  It may need updates more
   * often, because the variables that the label depends on may change
   * at any time.
   */
  [[gnu::pure]]
  bool IsDynamic() const noexcept {
    return label != nullptr && _tcsstr(label, _T("$(")) != nullptr;
  }
};

/**
 * A container for MenuItem objects.
 */
class Menu {
public:
  static constexpr std::size_t MAX_ITEMS = 32;

protected:
  std::array<MenuItem, MAX_ITEMS> items;

public:
  void Clear() noexcept;

  const MenuItem &operator[](unsigned i) const noexcept {
    return items[i];
  }

  void Add(const TCHAR *label, unsigned location, unsigned event_id) noexcept;

  [[gnu::pure]]
  int FindByEvent(unsigned event) const noexcept;
};

#endif
