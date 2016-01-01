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

#include "ComboList.hpp"
#include "Util/StringAPI.hxx"

#include <algorithm>

ComboList::Item::Item(int _int_value,
                      const TCHAR *_string_value,
                      const TCHAR *_display_string,
                      const TCHAR *_help_text)
  :int_value(_int_value),
   string_value(AllocatedString<TCHAR>::Duplicate(_string_value)),
   display_string(AllocatedString<TCHAR>::Duplicate(_display_string)),
   help_text(_help_text != nullptr
             ? AllocatedString<TCHAR>::Duplicate(_help_text)
             : nullptr)
{
}

void
ComboList::Sort()
{
  std::sort(items.begin(), items.end(), [](const Item &a, const Item &b){
      return StringCollate(a.display_string.c_str(),
                           b.display_string.c_str()) < 0;
    });
}

unsigned
ComboList::LookUp(int int_value)
{
  for (unsigned i = 0; i < items.size(); i++)
    if (items[i].int_value == int_value)
      return i;

  return 0;
}
