/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#include "GetJsonString.hpp"
#include "util/ConvertString.hpp"
#include "Language/Language.hpp"

#include <tchar.h>

// Wrapper for getting converted string values of a json string
const StaticString<0x40>
GetJsonString(boost::json::value json_value,
              std::string_view key) noexcept {
  StaticString<0x40> str;
  auto value = json_value.as_object().if_contains(key);
  if (value != nullptr)
    str = UTF8ToWideConverter(value->get_string().c_str());
  else
    str.Format(_T("'%s' %s"), UTF8ToWideConverter(key.data()).c_str(),
               _("not found"));
  return str;
}
