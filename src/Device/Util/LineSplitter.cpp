// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LineSplitter.hpp"
#include "util/TextFile.hxx"
#include "util/StringStrip.hxx"

#include <algorithm>

#include <string.h>

constexpr
static bool
IsInsaneChar(char ch)
{
  return (unsigned char)ch < 0x20;
}

/**
 * Replace all control characters with a regular space character.
 */
static void
SanitiseLine(char *const begin, char *const end)
{
  std::replace_if(begin, end, IsInsaneChar, ' ');
}

bool
PortLineSplitter::DataReceived(std::span<const std::byte> s) noexcept
{
  assert(!s.empty());

  const char *data = (const char *)s.data(), *end = data + s.size();

  do {
    /* append new data to buffer, as much as fits there */
    auto range = buffer.Write();
    if (range.empty()) {
      /* overflow: reset buffer to recover quickly */
      buffer.Clear();
      continue;
    }

    size_t nbytes = std::min(range.size(), size_t(end - data));
    memcpy(range.data(), data, nbytes);
    data += nbytes;
    buffer.Append(nbytes);

    while (true) {
      /* read data from the buffer, to see if there's a newline
         character */
      char *line = ReadBufferedLine(buffer);
      if (line == nullptr)
        /* no newline here: wait for more data */
        break;

      char *end = line + strlen(line);

      /* remove trailing whitespace, such as '\r' */
      end = StripRight(line, end);

      SanitiseLine(line, end);

      /* if there are NUL bytes in the line, skip to after the last
         one, to avoid conflicts with NUL terminated C strings due to
         binary garbage */
      void *nul;
      while ((nul = memchr(line, 0, end - line)) != nullptr)
        line = (char *)nul + 1;

      if (!LineReceived(line))
        return false;
    }
  } while (data < end);

  return true;
}
