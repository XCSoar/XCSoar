// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Enum.hpp"
#include "ComboList.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>

#include <stdlib.h>

DataFieldEnum::Entry::~Entry() noexcept
{
  free(string);

  if (display_string != string)
    free(display_string);

  free(help);
}

void
DataFieldEnum::Entry::SetString(const char *_string) noexcept
{
  free(string);
  if (display_string != string)
    free(display_string);

  display_string = string = strdup(_string);
}

void
DataFieldEnum::Entry::SetDisplayString(const char *_string) noexcept
{
  if (display_string != string)
    free(display_string);
  display_string = strdup(_string);
}

void
DataFieldEnum::Entry::Set(unsigned _id, const char *_string,
                          const char *_display_string,
                          const char *_help) noexcept
{
  id = _id;
  SetString(_string);

  if (_display_string != nullptr)
    display_string = strdup(_display_string);

  free(help);
  help = _help ? strdup(_help) : nullptr;
}

unsigned
DataFieldEnum::GetValue() const noexcept
{
  assert(value < entries.size());
  return entries[value].GetId();
}

void
DataFieldEnum::replaceEnumText(std::size_t i, const char *Text) noexcept
{
  if (i <= entries.size())
    entries[i].SetString(Text);
}

bool
DataFieldEnum::AddChoice(unsigned id, const char *text,
                         const char *display_string,
                         const char *help) noexcept
{
  if (entries.full())
    return false;

  Entry &entry = entries.append();
  entry.Set(id, text, display_string, help);
  return true;
}

void
DataFieldEnum::AddChoices(const StaticEnumChoice *p) noexcept
{
  while (p->display_string != nullptr) {
    const char *help = p->help;
    if (help != nullptr)
      help = gettext(help);

    AddChoice(p->id, gettext(p->display_string), nullptr, help);
    ++p;
  }
}

unsigned
DataFieldEnum::addEnumText(const char *Text, const char *display_string,
                           const char *_help) noexcept
{
  if (entries.full())
    return 0;

  unsigned i = entries.size();
  Entry &entry = entries.append();
  entry.Set(i, Text, display_string, _help);
  return i;
}

void
DataFieldEnum::addEnumTexts(const char *const*list) noexcept
{
  while (*list != nullptr)
    addEnumText(*list++);
}

const char *
DataFieldEnum::GetAsString() const noexcept
{
  if (entries.empty()) {
    assert(value == 0);
    return "";
  } else {
    assert(value < entries.size());
    return entries[value].GetString();
  }
}

const char *
DataFieldEnum::GetAsDisplayString() const noexcept
{
  if (entries.empty()) {
    assert(value == 0);
    return "";
  } else {
    assert(value < entries.size());
    return entries[value].GetDisplayString();
  }
}

const char *
DataFieldEnum::GetHelp() const noexcept
{
  if (entries.empty()) {
    return nullptr;
  } else {
    assert(value < entries.size());
    return entries[value].GetHelp();
  }
}

void
DataFieldEnum::SetValue(unsigned Value) noexcept
{
  int i = Find(Value);
  if (i >= 0)
    SetIndex(i, false);
}

bool
DataFieldEnum::SetValue(const char *text) noexcept
{
  int i = Find(text);
  if (i < 0)
    return false;

  SetIndex(i, false);
  return true;
}

bool
DataFieldEnum::ModifyValue(unsigned new_value) noexcept
{
  int i = Find(new_value);
  if (i < 0)
    return false;

  SetIndex(i, true);
  return true;
}

bool
DataFieldEnum::ModifyValue(const char *text) noexcept
{
  int i = Find(text);
  if (i < 0)
    return false;

  SetIndex(i, true);
  return true;
}

int
DataFieldEnum::SetStringAutoAdd(const char *text) noexcept
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
DataFieldEnum::Inc() noexcept
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
DataFieldEnum::Dec() noexcept
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
DataFieldEnum::Sort(std::size_t startindex) noexcept
{
  std::sort(std::next(entries.begin(), startindex), entries.end(),
            [](const DataFieldEnum::Entry &a, const DataFieldEnum::Entry &b) {
              return StringCollate(a.GetDisplayString(),
                                   b.GetDisplayString()) < 0;
            });
}

ComboList
DataFieldEnum::CreateComboList([[maybe_unused]] const char *reference_string) const noexcept
{
  ComboList combo_list;

  for (const auto &i : entries)
    combo_list.Append(i.GetId(), i.GetString(), i.GetDisplayString(),
                       i.GetHelp());

  combo_list.current_index = value;
  return combo_list;
}

void
DataFieldEnum::SetFromCombo(int i, const char *) noexcept
{
  ModifyValue(i);
}

int
DataFieldEnum::Find(const char *text) const noexcept
{
  assert(text != nullptr);

  for (std::size_t i = 0; i < entries.size(); i++)
    if (StringIsEqual(text, entries[i].GetString()))
      return i;

  return -1;
}

int
DataFieldEnum::Find(unsigned id) const noexcept
{
  for (std::size_t i = 0; i < entries.size(); i++)
    if (entries[i].GetId() == id)
      return i;

  return -1;
}

void
DataFieldEnum::SetIndex(std::size_t new_value, bool invoke_callback) noexcept
{
  assert(new_value < entries.size());

  if (new_value == value)
    return;

  value = new_value;

  if (invoke_callback)
    Modified();
}
