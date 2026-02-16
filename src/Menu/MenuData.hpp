// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <string.h>

#include <array>

/**
 * Data of an item in the mode menu.
 */
class MenuItem {
public:
  const char *label;
  unsigned event;

  void Clear() noexcept {
    label = nullptr;
    event = 0;
  }

  constexpr bool IsDefined() const noexcept {
    return event > 0;
  }

  /**
   * Does this item have a dynamic label?  It may need updates more
   * often, because the variables that the label depends on may change
   * at any time.
   */
  [[gnu::pure]]
  bool IsDynamic() const noexcept {
    return label != nullptr && strstr(label, _T("$(")) != nullptr;
  }
};

/**
 * A container for MenuItem objects.
 */
class Menu {
public:
  static constexpr std::size_t MAX_ITEMS = 64;

protected:
  std::array<MenuItem, MAX_ITEMS> items;

public:
  void Clear() noexcept;

  const MenuItem &operator[](unsigned i) const noexcept {
    return items[i];
  }

  void Add(const char *label, unsigned location, unsigned event_id) noexcept;

  [[gnu::pure]]
  int FindByEvent(unsigned event) const noexcept;
};
