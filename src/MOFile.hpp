/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_MO_FILE_HPP
#define XCSOAR_MO_FILE_HPP

#include "OS/FileMapping.hpp"
#include "Util/AllocatedArray.hpp"

#include <stdint.h>

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

  FileMapping mapping;

  bool native_byte_order;

  unsigned count;
  AllocatedArray<string_pair> strings;

public:
  MOFile(const TCHAR *path);

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

#endif
