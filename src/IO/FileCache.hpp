/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FILE_CACHE_HPP
#define XCSOAR_FILE_CACHE_HPP

#include <stdio.h>
#include <tchar.h>

class FileCache {
  TCHAR *cache_path;
  size_t cache_path_length;

public:
  FileCache(const TCHAR *_cache_path);
  ~FileCache();

protected:
  size_t path_buffer_size(const TCHAR *name) const;
  const TCHAR *make_cache_path(TCHAR *buffer, const TCHAR *name) const;

public:
  void flush(const TCHAR *name);
  FILE *load(const TCHAR *name, const TCHAR *original_path);

  FILE *save(const TCHAR *name, const TCHAR *original_path);
  bool commit(const TCHAR *name, FILE *file);
  void cancel(const TCHAR *name, FILE *file);
};

#endif
