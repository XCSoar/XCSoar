// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/PortableColor.hpp"
#include "ResourceId.hpp"

#include <optional>
#include <string_view>

struct TopographyIndexEntry {
  std::string_view name;

  double shape_range, label_range, important_label_range;

  ResourceId icon, big_icon, ultra_icon;

  long shape_field;

  BGRA8Color color;

  unsigned pen_width = 1;
};

std::optional<TopographyIndexEntry>
ParseTopographyIndexLine(const char *line) noexcept;
