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

#ifndef XCSOAR_DATA_FIELD_COMBO_LIST_HPP
#define XCSOAR_DATA_FIELD_COMBO_LIST_HPP

#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hpp"

#include <tchar.h>

class ComboList : private NonCopyable {
public:
  struct Item : private NonCopyable {
    enum {
      NEXT_PAGE = -800001,
      PREVIOUS_PAGE = -800002,
    };

    int DataFieldIndex;
    TCHAR *StringValue;
    TCHAR *StringValueFormatted;
    TCHAR *StringHelp;

    Item(int _DataFieldIndex, const TCHAR *_StringValue,
         const TCHAR *_StringValueFormatted, const TCHAR *_StringHelp = NULL);
    ~Item();
  };

  enum {
    MAX_SIZE = 300,
  };

  int ComboPopupItemSavedIndex;

private:
  StaticArray<Item*, MAX_SIZE> items;

public:
  ComboList()
    :ComboPopupItemSavedIndex(0) {}

  ~ComboList() {
    Clear();
  }

  unsigned size() const {
    return items.size();
  }

  const Item& operator[](unsigned i) const {
    return *items[i];
  }

  void Clear();

  unsigned Append(Item *item);

  unsigned Append(int DataFieldIndex,
                  const TCHAR *StringValue,
                  const TCHAR *StringValueFormatted,
                  const TCHAR *StringHelp = NULL) {
    return Append(new Item(DataFieldIndex,
                           StringValue, StringValueFormatted, StringHelp));
  }

  unsigned Append(int DataFieldIndex, const TCHAR *StringValue) {
    return Append(DataFieldIndex, StringValue, StringValue);
  }

  void Sort();
  unsigned LookUp(int DataFieldIndex);
};

#endif
