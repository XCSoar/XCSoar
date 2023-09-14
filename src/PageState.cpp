// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageState.hpp"

void
PagesState::Clear()
{
  current_index = 0;
  special_page.SetUndefined();

  for (auto &p : pages)
    p.Clear();
}
