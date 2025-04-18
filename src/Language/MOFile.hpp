// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/AllocatedArray.hxx"

#include <cstdint>

/**
 * Loader for GNU gettext *.mo files.
 */
class MOFile {
  struct mo_header {
    uint32_t magic;
    uint32_t format_revision;
    uint32_t num_strings;
    uint32_t original_table_offset, translation_table_offset;
    uint32_t hash_table_size, hash_table_offset;
  };

  struct mo_table_entry {
    uint32_t length;
    uint32_t offset;
  };

  struct string_pair {
    const char *original, *translation;
  };

  std::span<const std::byte> raw;

  bool native_byte_order;

  unsigned count;
  AllocatedArray<string_pair> strings;

public:
  explicit MOFile(std::span<const std::byte> _raw);

  bool error() const {
    return count == 0;
  }

  const char *lookup(const char *p) const;

private:
  uint32_t import_uint32(uint32_t x) const {
    return native_byte_order
      ? x
      : ((x >> 24) | ((x >> 8) & 0xff00) |
         ((x << 8) & 0xff0000) | (x << 24));
  }

  const char *get_string(const struct mo_table_entry *entry) const;
};
