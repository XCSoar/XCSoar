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

#include "Enum.hpp"
#include "ComboList.hpp"
#include "Language/Language.hpp"
#include "Util/StringAPI.hxx"

#include <algorithm>

#include <stdlib.h>

DataFieldEnum::Entry::~Entry()
{
  free(string);

  if (display_string != string)
    free(display_string);

  free(help);
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

  if (_display_string != nullptr)
    display_string = _tcsdup(_display_string);

  free(help);
  help = _help ? _tcsdup(_help) : nullptr;
}

unsigned
DataFieldEnum::GetValue() const
{
  assert(value < entries.size());
  return entries[value].GetId();
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
  while (p->display_string != nullptr) {
    const TCHAR *help = p->help;
    if (help != nullptr)
      help = gettext(help);

    AddChoice(p->id, gettext(p->display_string), nullptr, help);
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
  while (*list != nullptr)
    addEnumText(*list++);
}

const TCHAR *
DataFieldEnum::GetAsString() const
{
  if (entries.empty()) {
    assert(value == 0);
    return _T("");
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
    return _T("");
  } else {
    assert(value < entries.size());
    return entries[value].GetDisplayString();
  }
}

const TCHAR *
DataFieldEnum::GetHelp() const
{
  if (entries.empty()) {
    return nullptr;
  } else {
    assert(value < entries.size());
    return entries[value].GetHelp();
  }
}

void
DataFieldEnum::Set(unsigned Value)
{
  int i = Find(Value);
  if (i >= 0)
    SetIndex(i, false);
}

bool
DataFieldEnum::Set(const TCHAR *text)
{
  int i = Find(text);
  if (i < 0)
    return false;

  SetIndex(i, false);
  return true;
}

int
DataFieldEnum::SetStringAutoAdd(const TCHAR *text)
{
  int index = Find(text);
  if (index >= 0) {
    SetIndex(index, false);
    return entries[index].GetId();
  } else {
    index = entries.size();
    unsigned id = addEnumText(text);
    SetIndex(index, false);
    return id;
  }
}

void
DataFieldEnum::SetAsInteger(int Value)
{
  int i = Find(Value);
  if (i >= 0)
    SetIndex(i, true);
}

void
DataFieldEnum::SetAsString(const TCHAR *Value)
{
  int i = Find(Value);
  if (i >= 0)
    SetIndex(i, true);
}

void
DataFieldEnum::Inc()
{
  if (entries.empty()) {
    assert(value == 0);
    return;
  }

  assert(value < entries.size());

  if (value < entries.size() - 1) {
    value++;
    Modified();
  }
}

void
DataFieldEnum::Dec()
{
  if (entries.empty()) {
    assert(value == 0);
    return;
  }

  assert(value < entries.size());

  if (value > 0) {
    value--;
    Modified();
  }
}

void
DataFieldEnum::Sort(unsigned startindex)
{
  std::sort(entries.begin() + startindex, entries.end(),
            [](const DataFieldEnum::Entry &a, const DataFieldEnum::Entry &b) {
              return StringCollate(a.GetDisplayString(),
                                   b.GetDisplayString()) < 0;
            });
}

ComboList
DataFieldEnum::CreateComboList(const TCHAR *reference_string) const
{
  ComboList combo_list;

  for (const auto &i : entries)
    combo_list.Append(i.GetId(), i.GetString(), i.GetDisplayString(),
                       i.GetHelp());

  combo_list.current_index = value;
  return combo_list;
}

int
DataFieldEnum::Find(const TCHAR *text) const
{
  assert(text != nullptr);

  for (unsigned int i = 0; i < entries.size(); i++)
    if (StringIsEqual(text, entries[i].GetString()))
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

  if (invoke_callback)
    Modified();
}

unsigned
DataFieldEnum::getItem(unsigned index) const
{
  return entries[index].GetId();
}
