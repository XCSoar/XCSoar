// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

#include <tchar.h>

class Widget;

struct TabMenuPage {
  const TCHAR *menu_caption;

  std::unique_ptr<Widget> (*Load)();
};

struct TabMenuGroup {
  const TCHAR *caption;

  const TabMenuPage *pages;
};
