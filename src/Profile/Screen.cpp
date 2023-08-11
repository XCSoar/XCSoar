// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "Formatter/HexColor.hpp"
#include "util/Macros.hpp"

void
ProfileMap::SetColor(std::string_view key, const RGB8Color color) noexcept
{
  char buffer[16];
  FormatHexColor(buffer, ARRAY_SIZE(buffer), color);
  Set(key, buffer);
}

bool
ProfileMap::GetColor(std::string_view key, RGB8Color &color) const noexcept
{
  const char *color_string = Get(key);
  if (!color_string)
    return false;

  return ParseHexColor(color_string, color);
}
