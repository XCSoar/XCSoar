// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MenuData.hpp"

void
Menu::Clear() noexcept
{
  for (auto &i : items)
    i.Clear();
}

void
Menu::Add(const char *label, unsigned location, unsigned event_id) noexcept
{
  if (location >= items.size())
    return;

  MenuItem &item = items[location];

  item.label = label;
  item.event = event_id;
}

int
Menu::FindByEvent(unsigned event) const noexcept
{
  for (std::size_t i = 0; i < items.size(); ++i)
    if (items[i].event == event)
      return i;

  return -1;
}
