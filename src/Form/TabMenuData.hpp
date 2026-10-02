// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;

struct TabMenuPage {
  const char *menu_caption;

  std::unique_ptr<Widget> (*Load)();

  /** Called when building the menu; an unavailable page is omitted. */
  bool (*IsAvailable)() = nullptr;
};

struct TabMenuGroup {
  const char *caption;

  const TabMenuPage *pages;
};
