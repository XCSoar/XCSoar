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

#include "DataField/ComboList.hpp"

#include <stdlib.h>
#include <string.h>

ComboList::Item::Item(int _DataFieldIndex,
                      const TCHAR *_StringValue,
                      const TCHAR *_StringValueFormatted,
                      const TCHAR *_StringHelp)
  :DataFieldIndex(_DataFieldIndex),
   StringValue(_tcsdup(_StringValue)),
   StringValueFormatted(_tcsdup(_StringValueFormatted)),
   StringHelp(_StringHelp ? _tcsdup(_StringHelp) : NULL)
{
}

ComboList::Item::~Item()
{
  free(StringValue);
  free(StringValueFormatted);
  free(StringHelp);
}

void
ComboList::Clear()
{
  for (auto it = items.begin(), end = items.end(); it != end; ++it)
    delete *it;

  items.clear();
}

unsigned
ComboList::Append(ComboList::Item *item)
{
  unsigned i = items.size();
  items.append(item);
  return i;
}

static int
CompareByValue(const void *elem1, const void *elem2)
{
  const ComboList::Item* entry1 = *((const ComboList::Item**)elem1);
  const ComboList::Item* entry2 = *((const ComboList::Item**)elem2);

  return _tcscmp(entry1->StringValueFormatted, entry2->StringValueFormatted);
}

void
ComboList::Sort()
{
  qsort(items.begin(), items.size(), sizeof(items[0]), CompareByValue);
}

unsigned
ComboList::LookUp(int DataFieldIndex)
{
  for (unsigned i = 0; i < items.size(); i++)
    if (items[i]->DataFieldIndex == DataFieldIndex)
      return i;

  return 0;
}
