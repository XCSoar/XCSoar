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

#include "FilePicker.hpp"
#include "ComboPicker.hpp"
#include "Form/DataField/FileReader.hpp"
#include "Form/DataField/ComboList.hpp"

bool
FilePicker(const TCHAR *caption, const TCHAR *patterns, TCHAR *buffer)
{
  assert(patterns != NULL);

  DataFieldFileReader df;
  df.ScanMultiplePatterns(patterns);
  const ComboList combo_list = df.CreateComboList(nullptr);
  if (combo_list.size() == 0)
    return false;

  int i = ComboPicker(caption, combo_list, nullptr);
  if (i < 0)
    return false;

  const ComboList::Item &item = combo_list[i];
  df.SetFromCombo(item.DataFieldIndex, item.StringValue);

  _tcscpy(buffer, df.GetAsString());
  return true;
}
