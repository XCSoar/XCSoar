// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "LogFile.hpp"

#include <algorithm>
#include <cstddef>
#include <span>

static constexpr bool
IsPrintable(std::byte ch) noexcept
{
  return ch >= std::byte{0x20} && ch < std::byte{0x80};
}

static inline void
HexDumpLine(const char *prefix, unsigned offset,
            std::span<const std::byte> src) noexcept
{
  StaticString<128> line;
  line.clear();

  for (size_t i = 0; i < src.size(); ++i) {
    if ((i & 0x7) == 0)
      line += " ";

    line.AppendFormat(" %02x", static_cast<unsigned>(src[i]));
  }

  for (size_t i = src.size(); i < 0x10; ++i) {
    if ((i & 0x7) == 0)
      line += " ";

    line += "   ";
  }

  line += " ";
  for (size_t i = 0; i < src.size(); ++i) {
    if ((i & 0x7) == 0)
      line += " ";

    char byte[2];
    byte[0] = IsPrintable(src[i]) ? static_cast<char>(src[i]) : '.';
    byte[1] = '\0';
    line += byte;
  }

  LogFormat("%s%04x%s", prefix, offset, line.c_str());
}

static inline void
HexDump(const char *prefix, std::span<const std::byte> src) noexcept
{
  unsigned offset = 0;
  while (!src.empty()) {
    size_t line_length = std::min(src.size(), std::size_t{0x10});
    HexDumpLine(prefix, offset, src.first(line_length));
    src = src.subspan(line_length);
    offset += 0x10;
  }
}

static inline void
HexDump(std::span<const std::byte> src) noexcept
{
  HexDump("", src);
}
