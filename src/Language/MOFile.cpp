/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "MOFile.hpp"

#include <assert.h>
#include <string.h>

MOFile::MOFile(const void *_data, size_t _size)
  :data((const uint8_t *)_data), size(_size), count(0) {
  const struct mo_header *header = (const struct mo_header *)_data;
  if (size < sizeof(*header))
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
    (const void *)(data + import_uint32(header->original_table_offset));
  for (unsigned i = 0; i < n; ++i) {
    strings[i].original = get_string(entry++);
    if (strings[i].original == NULL)
      return;
  }

  entry = (const struct mo_table_entry *)(const void *)
    (data + import_uint32(header->translation_table_offset));
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

  if (offset >= size || length >= size || (offset + length) >= size)
    /* overflow */
    return NULL;

  const char *p = (const char *)(data + offset);
  if (p[length] != 0 || strlen(p) != length)
    /* invalid string */
    return NULL;

  return p;
}
