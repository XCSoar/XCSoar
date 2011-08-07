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

#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "Language/Language.hpp"

#include <stdlib.h>

#ifndef WIN32
#define _cdecl
#endif

DataFieldEnum::Entry::~Entry()
{
  free(string);

  if (display_string != string)
    free(display_string);

  free(mHelp);
}

void
DataFieldEnum::Entry::SetString(const TCHAR *_string)
{
  free(string);
  if (display_string != string)
    free(display_string);

  display_string = string = _tcsdup(_string);
}

void
DataFieldEnum::Entry::Set(unsigned _id, const TCHAR *_string,
                          const TCHAR *_display_string,
                          const TCHAR *_help)
{
  id = _id;
  SetString(_string);

  if (_display_string != NULL)
    display_string = _tcsdup(_display_string);

  free(mHelp);
  mHelp = _help ? _tcsdup(_help) : NULL;
}

int
DataFieldEnum::GetAsInteger() const
{
  if (entries.empty()) {
    assert(value == 0);
    return 0;
  } else {
    assert(value < entries.size());
    return entries[value].GetId();
  }
}

void
DataFieldEnum::replaceEnumText(unsigned int i, const TCHAR *Text)
{
  if (i <= entries.size())
    entries[i].SetString(Text);
}

bool
DataFieldEnum::AddChoice(unsigned id, const TCHAR *text,
                         const TCHAR *display_string, const TCHAR *help)
{
  if (entries.full())
    return false;

  Entry &entry = entries.append();
  entry.Set(id, text, display_string, help);
  return true;
}

void
DataFieldEnum::AddChoices(const StaticEnumChoice *p)
{
  while (p->display_string != NULL) {
    const TCHAR *help = p->help;
    if (help != NULL)
      help = gettext(help);

    AddChoice(p->id, gettext(p->display_string), NULL, help);
    ++p;
  }
}

unsigned
DataFieldEnum::addEnumText(const TCHAR *Text, const TCHAR *display_string,
                           const TCHAR *_help)
{
  if (entries.full())
    return 0;

  unsigned i = entries.size();
  Entry &entry = entries.append();
  entry.Set(i, Text, display_string, _help);
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
  if (entries.empty()) {
    assert(value == 0);
    return NULL;
  } else {
    assert(value < entries.size());
    return entries[value].GetString();
  }
}

const TCHAR *
DataFieldEnum::GetAsDisplayString() const
{
  if (entries.empty()) {
    assert(value == 0);
    return NULL;
  } else {
    assert(value < entries.size());
    return entries[value].GetDisplayString();
  }
}

void
DataFieldEnum::Set(int Value)
{
  int i = Find(Value);
  if (i >= 0)
    SetIndex(i, false);
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
  if (i >= 0)
    SetIndex(i, true);
}

void
DataFieldEnum::Inc(void)
{
  if (entries.empty()) {
    assert(value == 0);
    return;
  }

  assert(value < entries.size());

  if (value < entries.size() - 1) {
    value++;
    if (!GetDetachGUI() && mOnDataAccess != NULL)
      (mOnDataAccess)(this, daChange);
  }
}

void
DataFieldEnum::Dec(void)
{
  if (entries.empty()) {
    assert(value == 0);
    return;
  }

  assert(value < entries.size());

  if (value > 0) {
    value--;
    if (!GetDetachGUI() && mOnDataAccess != NULL)
      (mOnDataAccess)(this, daChange);
  }
}

static int _cdecl
DataFieldEnumCompare(const void *elem1, const void *elem2)
{
  const DataFieldEnum::Entry *entry1 = (const DataFieldEnum::Entry *)elem1;
  const DataFieldEnum::Entry *entry2 = (const DataFieldEnum::Entry *)elem2;

  return _tcscmp(entry1->GetDisplayString(), entry2->GetDisplayString());
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
    combo_list->Append(entries[i].GetId(), entries[i].GetString(),
                       entries[i].GetDisplayString(),
                       entries[i].GetHelp());

  combo_list->ComboPopupItemSavedIndex = value;
  return combo_list;
}

int
DataFieldEnum::Find(const TCHAR *text) const
{
  assert(text != NULL);

  for (unsigned int i = 0; i < entries.size(); i++)
    if (_tcscmp(text, entries[i].GetString()) == 0)
      return i;

  return -1;
}

int
DataFieldEnum::Find(unsigned id) const
{
  for (unsigned i = 0; i < entries.size(); i++)
    if (entries[i].GetId() == id)
      return i;

  return -1;
}

void
DataFieldEnum::SetIndex(unsigned new_value, bool invoke_callback)
{
  assert(new_value < entries.size());

  if (new_value == value)
    return;

  value = new_value;
  if (invoke_callback && !GetDetachGUI() && mOnDataAccess != NULL)
    mOnDataAccess(this, daChange);
}

unsigned
DataFieldEnum::getItem(unsigned index) const
{
  return entries[index].GetId();
}
