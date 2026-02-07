// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ComboList.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>

ComboList::Item::Item(int _int_value,
                      const char *_string_value,
                      const char *_display_string,
                      const char *_help_text) noexcept
  :int_value(_int_value),
   string_value(_string_value),
   display_string(_display_string),
   help_text(_help_text != nullptr
             ? _help_text
             : "")
{
}

void
ComboList::Sort() noexcept
{
  std::sort(items.begin(), items.end(), [](const Item &a, const Item &b){
      return StringCollate(a.display_string.c_str(),
                           b.display_string.c_str()) < 0;
    });
}

unsigned
ComboList::LookUp(int int_value) noexcept
{
  for (unsigned i = 0; i < items.size(); i++)
    if (items[i].int_value == int_value)
      return i;

  return 0;
}
