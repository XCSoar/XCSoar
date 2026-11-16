// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ByteSizeFormatter.hpp"
#include "util/Macros.hpp"
#include "util/StringFormat.hpp"

#include <cassert>

void
FormatByteSize(char *buffer, size_t size, uint64_t bytes, bool simple)
{
  assert(buffer != NULL);
  assert(size >= 8);

  static const char *const units[] = { "B", "KB", "MB", "GB", "TB" };
  static const char *const simple_units[] = { "B", "K", "M", "G", "T" };

  double value = bytes;

  unsigned i = 0;
  for (; value >= 1024 && i < ARRAY_SIZE(units)-1; i++, value /= 1024);

  const char *unit = simple ? simple_units[i] : units[i];

  const char *format;
  if (value >= 100 || i == 0)
    format = simple ? "%.0f%s" : "%.0f %s";
  else if (value >= 10)
    format = simple ? "%.1f%s" : "%.1f %s";
  else
    format = simple ? "%.1f%s" : "%.2f %s";

  StringFormat(buffer, size, format, value, unit);
}

static bool
BarShowsBytes(unsigned done, unsigned total, bool bytes,
              unsigned byte_count) noexcept
{
  /* A percent or block counter can carry a separate byte rate.
     Label the amount only when the bar itself is that byte count.
     0..100 while both are still zero is a percent. */
  return bytes && done == byte_count && (done > 0 || total >= 1024u);
}

void
FormatProgressAmount(char *buffer, size_t size,
                     unsigned done, unsigned total,
                     bool bytes, unsigned byte_count) noexcept
{
  assert(buffer != NULL);
  assert(size >= 8);

  if (!BarShowsBytes(done, total, bytes, byte_count)) {
    StringFormat(buffer, size, "%u/%u", done, total);
    return;
  }

  char done_s[16], total_s[16];
  FormatByteSize(done_s, sizeof(done_s), done);
  FormatByteSize(total_s, sizeof(total_s), total);
  StringFormat(buffer, size, "%s/%s", done_s, total_s);
}
