/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"

#include <stdlib.h>

#ifndef WIN32
#define _cdecl
#endif

DataFieldEnum::Entry::~Entry()
{
  free(mText);
}

int
DataFieldEnum::GetAsInteger() const
{
  if (mValue < entries.size())
    return entries[mValue].index;

  return 0;
}

void
DataFieldEnum::replaceEnumText(unsigned int i, const TCHAR *Text)
{
  if (i <= entries.size()) {
    free(entries[i].mText);
    entries[i].mText = _tcsdup(Text);
 }
}

unsigned
DataFieldEnum::addEnumText(const TCHAR *Text)
{
  if (entries.full())
    return 0;

  unsigned i = entries.size();
  Entry &entry = entries.append();
  entry.mText = _tcsdup(Text);
  entry.index = i;
  return i;
}

void
DataFieldEnum::addEnumTexts(const TCHAR *const*list)
{
  while (*list != NULL)
    addEnumText(*list++);
}

const TCHAR *
DataFieldEnum::GetAsString() const
{
  if (mValue < entries.size())
    return entries[mValue].mText;

  return NULL;
}

void
DataFieldEnum::Set(int Value)
{
  // first look it up
  if (Value < 0)
    Value = 0;

  for (unsigned int i = 0; i < entries.size(); i++) {
    if (entries[i].index == (unsigned int)Value) {
      int lastValue = mValue;
      mValue = i;

      if (mValue != (unsigned int)lastValue) {
        if (!GetDetachGUI())
          (mOnDataAccess)(this, daChange);
      }
      return;
    }
  }
  mValue = 0; // fallback
}

void
DataFieldEnum::SetAsInteger(int Value)
{
  Set(Value);
}

void
DataFieldEnum::SetAsString(const TCHAR *Value)
{
  int i = Find(Value);
  if (i >= 0) {
    if ((unsigned)i != mValue) {
      mValue = i;
      if (!GetDetachGUI())
        (mOnDataAccess)(this, daChange);
    }
  } else
    mValue = 0; // fallback
}

void
DataFieldEnum::Inc(void)
{
  if (mValue < entries.size() - 1) {
    mValue++;
    if (!GetDetachGUI())
      (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldEnum::Dec(void)
{
  if (mValue > 0) {
    mValue--;
    if (!GetDetachGUI())
      (mOnDataAccess)(this, daChange);
  }
}

static int _cdecl
DataFieldEnumCompare(const void *elem1, const void *elem2)
{
  const DataFieldEnum::Entry *entry1 = (const DataFieldEnum::Entry *)elem1;
  const DataFieldEnum::Entry *entry2 = (const DataFieldEnum::Entry *)elem2;

  return _tcscmp(entry1->mText, entry2->mText);
}

void
DataFieldEnum::Sort(int startindex)
{
  qsort(entries.begin() + startindex, entries.size() - startindex,
        sizeof(entries[0]),
        DataFieldEnumCompare);
}

ComboList *
DataFieldEnum::CreateComboList() const
{
  ComboList *combo_list = new ComboList();

  for (unsigned i = 0; i < entries.size(); i++)
    combo_list->Append(i, entries[i].index,
                       entries[i].mText, entries[i].mText);

  combo_list->ComboPopupItemSavedIndex = mValue;
  return combo_list;
}

int
DataFieldEnum::Find(const TCHAR *text) const
{
  assert(text != NULL);

  for (unsigned int i = 0; i < entries.size(); i++)
    if (_tcscmp(text, entries[i].mText) == 0)
      return entries[i].index;

  return -1;
}
