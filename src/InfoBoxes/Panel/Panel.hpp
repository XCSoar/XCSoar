// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>
#include <tchar.h>

class Widget;

struct InfoBoxPanel {
  constexpr
  InfoBoxPanel(const char *_name, std::unique_ptr<Widget> (*_load)(unsigned id))
    :name(_name), load(_load) {};

  const char *name;
  std::unique_ptr<Widget> (*load)(unsigned id);
};
