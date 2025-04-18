// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MOFile.hpp"

#include <cassert>
#include <string.h>

MOFile::MOFile(std::span<const std::byte> _raw)
  :raw(_raw), count(0) {
  const struct mo_header *header = (const struct mo_header *)(const void *)raw.data();
  if (raw.size() < sizeof(*header))
    return;

  if (header->magic == 0x950412de)
    native_byte_order = true;
  else if (header->magic == 0xde120495)
    native_byte_order = false;
  else
    /* invalid magic */
    return;

  unsigned n = import_uint32(header->num_strings);
  if (n >= 0x100000)
    return;

  strings.ResizeDiscard(n);

  const struct mo_table_entry *entry = (const struct mo_table_entry *)
    (const void *)(raw.data() + import_uint32(header->original_table_offset));
  for (unsigned i = 0; i < n; ++i) {
    strings[i].original = get_string(entry++);
    if (strings[i].original == NULL)
      return;
  }

  entry = (const struct mo_table_entry *)(const void *)
    (raw.data() + import_uint32(header->translation_table_offset));
  for (unsigned i = 0; i < n; ++i) {
    strings[i].translation = get_string(entry++);
    if (strings[i].translation == NULL)
      return;
  }

  count = n;
}

const char *
MOFile::lookup(const char *p) const
{
  assert(p != NULL);

  for (unsigned i = 0; i < count; ++i)
    if (strcmp(strings[i].original, p) == 0)
      return strings[i].translation;

  return NULL;
}

const char *
MOFile::get_string(const struct mo_table_entry *entry) const
{
  unsigned length = import_uint32(entry->length);
  unsigned offset = import_uint32(entry->offset);

  if (offset >= raw.size() || length >= raw.size() ||
      (offset + length) >= raw.size())
    /* overflow */
    return NULL;

  const char *p = (const char *)(raw.data() + offset);
  if (p[length] != 0 || strlen(p) != length)
    /* invalid string */
    return NULL;

  return p;
}
