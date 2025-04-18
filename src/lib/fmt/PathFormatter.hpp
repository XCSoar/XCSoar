// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <fmt/format.h>

template<>
struct fmt::formatter<Path> : formatter<string_view>
{
  template <typename FormatContext>
  auto format(Path path, FormatContext &ctx) const
  {
    return formatter<string_view>::format(path.ToUTF8(), ctx);
  }
};

template<>
struct fmt::formatter<AllocatedPath> : formatter<Path> {};
