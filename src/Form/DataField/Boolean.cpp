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

#include "Boolean.hpp"
#include "ComboList.hpp"

ComboList
DataFieldBoolean::CreateComboList([[maybe_unused]] const TCHAR *reference) const noexcept
{
  ComboList combo_list;
  combo_list.Append(false, false_text);
  combo_list.Append(true, true_text);

  combo_list.current_index = GetValue();
  return combo_list;
}

void
DataFieldBoolean::SetFromCombo(int i, const TCHAR *) noexcept
{
  ModifyValue(i != 0);
}

const TCHAR *
DataFieldBoolean::GetAsString() const noexcept
{
  return mValue ? true_text : false_text;
}

void
DataFieldBoolean::Inc() noexcept
{
  ModifyValue(true);
}

void
DataFieldBoolean::Dec() noexcept
{
  ModifyValue(false);
}
