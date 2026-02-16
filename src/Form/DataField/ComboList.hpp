// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/tstring.hpp"

#include <vector>
#include <tchar.h>

class ComboList {
public:
  struct Item {
    static constexpr int NEXT_PAGE = -800001;
    static constexpr int PREVIOUS_PAGE = -800002;
    static constexpr int DOWNLOAD = -800003;

    int int_value;
    tstring string_value;
    tstring display_string;
    tstring help_text;

    Item(int _int_value, const char *_string_value,
         const char *_display_string,
         const char *_help_text = nullptr) noexcept;

    Item(const Item &other) = delete;
    Item &operator=(const Item &other) = delete;

    Item(Item &&src) = default;
    Item &operator=(Item &&src) = default;
  };

  static constexpr unsigned MAX_SIZE = 512;

  int current_index = 0;

private:
  std::vector<Item> items;

public:
  ComboList() = default;

  ComboList(ComboList &&) = default;
  ComboList &operator=(ComboList &&) = default;

  bool empty() const noexcept {
    return items.empty();
  }

  unsigned size() const noexcept {
    return items.size();
  }

  const Item &operator[](unsigned i) const noexcept {
    return items[i];
  }

  int Find(int int_value) const noexcept {
    for (std::size_t i = 0; i < items.size(); ++i)
      if (items[i].int_value == int_value)
        return i;

    return -1;
  }

  void Clear() noexcept {
    items.clear();
  }

  unsigned Append(int int_value,
                  const char *string_value,
                  const char *display_string,
                  const char *help_text = nullptr) noexcept {
    unsigned i = items.size();
    items.emplace_back(int_value,
                       string_value, display_string, help_text);
    return i;
  }

  unsigned Append(const char *string_value,
                  const char *display_string,
                  const char *help_text = nullptr) noexcept {
    return Append(items.size(), string_value, display_string, help_text);
  }

  unsigned Append(int int_value, const char *string_value) noexcept {
    return Append(int_value, string_value, string_value);
  }

  unsigned Append(const char *string_value) noexcept {
    return Append(string_value, string_value);
  }

  void Sort() noexcept;
  unsigned LookUp(int int_value) noexcept;
};
