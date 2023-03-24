// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "LogFile.hpp"

#include <cstdint>

static inline bool
IsPrintable(uint8_t ch)
{
  return ch >= 0x20 && ch < 0x80;
}

static inline void
HexDumpLine(const char *prefix, unsigned offset,
            const uint8_t *data, size_t length)
{
  NarrowString<128> line;
  line.clear();

  for (size_t i = 0; i < length; ++i) {
    if ((i & 0x7) == 0)
      line += " ";

    line.AppendFormat(" %02x", data[i]);
  }

  for (size_t i = length; i < 0x10; ++i) {
    if ((i & 0x7) == 0)
      line += " ";

    line += "   ";
  }

  line += " ";
  for (size_t i = 0; i < length; ++i) {
    if ((i & 0x7) == 0)
      line += " ";

    char byte[2];
    byte[0] = IsPrintable(data[i]) ? (char)data[i] : '.';
    byte[1] = '\0';
    line += byte;
  }

  LogFormat("%s%04x%s", prefix, offset, line.c_str());
}

static inline void
HexDump(const char *prefix, const void *_data, size_t length)
{
  const uint8_t *data = (const uint8_t *)_data;
  unsigned offset = 0;
  while (length > 0) {
    size_t line_length = length;
    if (line_length > 0x10)
      line_length = 0x10;
    HexDumpLine(prefix, offset, data, line_length);
    data += line_length;
    length -= line_length;
    offset += 0x10;
  }
}

static inline void
HexDump(const void *data, size_t length)
{
  HexDump("", data, length);
}
