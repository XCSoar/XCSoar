/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Util/StaticArray.hxx"

#include <utility>

#include <tchar.h>

class ComboList {
public:
  struct Item {
    static constexpr int NEXT_PAGE = -800001;
    static constexpr int PREVIOUS_PAGE = -800002;
    static constexpr int DOWNLOAD = -800003;

    int int_value;
    TCHAR *string_value;
    TCHAR *display_string;
    TCHAR *help_text;

    Item(int _int_value, const TCHAR *_string_value,
         const TCHAR *_display_string, const TCHAR *_help_text = nullptr);
    ~Item();

    Item(const Item &other) = delete;
    Item &operator=(const Item &other) = delete;

    Item(Item &&src)
      :int_value(src.int_value), string_value(src.string_value),
       display_string(src.display_string), help_text(src.help_text) {
      src.string_value = src.display_string = src.help_text = nullptr;
    }

    Item &operator=(Item &&src) {
      int_value = src.int_value;
      std::swap(string_value, src.string_value);
      std::swap(display_string, src.display_string);
      std::swap(help_text, src.help_text);
      return *this;
    }

    friend void swap(Item &a, Item &b) {
      std::swap(a.int_value, b.int_value);
      std::swap(a.string_value, b.string_value);
      std::swap(a.display_string, b.display_string);
      std::swap(a.help_text, b.help_text);
    }
  };

  static constexpr unsigned MAX_SIZE = 512;

  int current_index;

private:
  StaticArray<Item*, MAX_SIZE> items;

public:
  ComboList()
    :current_index(0) {}

  ComboList(ComboList &&other);

  ~ComboList() {
    Clear();
  }

  ComboList(const ComboList &other) = delete;
  ComboList &operator=(const ComboList &other) = delete;

  bool empty() const {
    return items.empty();
  }

  unsigned size() const {
    return items.size();
  }

  const Item& operator[](unsigned i) const {
    return *items[i];
  }

  void Clear();

  unsigned Append(Item *item);

  unsigned Append(int int_value,
                  const TCHAR *string_value,
                  const TCHAR *display_string,
                  const TCHAR *help_text = nullptr) {
    return Append(new Item(int_value,
                           string_value, display_string, help_text));
  }

  unsigned Append(int int_value, const TCHAR *string_value) {
    return Append(int_value, string_value, string_value);
  }

  void Sort();
  unsigned LookUp(int int_value);
};

#endif
