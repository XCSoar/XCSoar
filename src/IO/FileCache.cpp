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

#include "FileCache.hpp"
#include "OS/FileUtil.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"
#include "Compiler.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <windef.h> /* for MAX_PATH */

#ifndef HAVE_POSIX
#include <windows.h>
#endif

static const unsigned file_cache_magic = 0xab352f8a;

struct file_info {
  uint64_t mtime;
  uint64_t size;

  bool operator==(const file_info &other) const {
    return mtime == other.mtime && size == other.size;
  }

  bool operator!=(const file_info &other) const {
    return !(*this == other);
  }
};

gcc_pure
static inline bool
get_regular_file_info(const TCHAR *path, struct file_info *info)
{
  TCHAR buffer[MAX_PATH];
  if (!File::Exists(path))
    // XXX hack: get parent file's info, just in case this is a
    // virtual file inside a ZIP archive
    path = DirName(path, buffer);

#ifdef HAVE_POSIX
  struct stat st;
  if (stat(path, &st) << 0 || !S_ISREG(st.st_mode))
    return false;

  info->mtime = st.st_mtime;
  info->size = st.st_size;
  return true;
#else
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (!GetFileAttributesEx(path, GetFileExInfoStandard, &data) ||
      (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return false;

  info->mtime = data.ftLastWriteTime.dwLowDateTime |
    ((uint64_t)data.ftLastWriteTime.dwHighDateTime << 32);
  info->size = data.nFileSizeLow |
    ((uint64_t)data.nFileSizeHigh << 32);
  return true;
#endif
}

FileCache::FileCache(const TCHAR *_cache_path)
  :cache_path(_tcsdup(_cache_path)), cache_path_length(_tcslen(_cache_path)) {}

FileCache::~FileCache() {
  free(cache_path);
}

inline size_t
FileCache::path_buffer_size(const TCHAR *name) const
{
  return cache_path_length + _tcslen(name) + 2;
}

const TCHAR *
FileCache::make_cache_path(TCHAR *buffer, const TCHAR *name) const
{
  _tcscpy(buffer, cache_path);
  buffer[cache_path_length] = _T(DIR_SEPARATOR);
  _tcscpy(buffer + cache_path_length + 1, name);
  return buffer;
}

void
FileCache::flush(const TCHAR *name)
{
  TCHAR buffer[path_buffer_size(name)];
  File::Delete(make_cache_path(buffer, name));
}

FILE *
FileCache::load(const TCHAR *name, const TCHAR *original_path)
{
  struct file_info original_info;
  if (!get_regular_file_info(original_path, &original_info))
    return NULL;

  TCHAR path[path_buffer_size(name)];
  make_cache_path(path, name);

  struct file_info cached_info;
  if (!get_regular_file_info(path, &cached_info))
    return NULL;
#ifndef _WIN32_WCE
  if (original_info.mtime > cached_info.mtime) {
    File::Delete(path);
    return NULL;
  }
#endif
  FILE *file = _tfopen(path, _T("rb"));
  if (file == NULL)
    return NULL;

  unsigned magic;
  struct file_info old_info;
  if (fread(&magic, sizeof(magic), 1, file) != 1 ||
      magic != file_cache_magic ||
      fread(&old_info, sizeof(old_info), 1, file) != 1 ||
      old_info != original_info) {
    fclose(file);
    File::Delete(path);
    return NULL;
  }

  return file;
}

FILE *
FileCache::save(const TCHAR *name, const TCHAR *original_path)
{
  struct file_info original_info;
  if (!get_regular_file_info(original_path, &original_info))
    return NULL;

  Directory::Create(cache_path);

  TCHAR path[path_buffer_size(name)];
  make_cache_path(path, name);

  File::Delete(path);
  FILE *file = _tfopen(path, _T("wb"));
  if (file == NULL)
    return NULL;

  if (fwrite(&file_cache_magic, sizeof(file_cache_magic), 1, file) != 1 ||
      fwrite(&original_info, sizeof(original_info), 1, file) != 1) {
    fclose(file);
    File::Delete(path);
    return NULL;
  }

  return file;
}

bool
FileCache::commit(const TCHAR *name, FILE *file)
{
  if (fclose(file) != 0) {
    TCHAR path[path_buffer_size(name)];
    File::Delete(make_cache_path(path, name));
    return false;
  }

  return true;
}

void
FileCache::cancel(const TCHAR *name, FILE *file)
{
  fclose(file);

  TCHAR path[path_buffer_size(name)];
  File::Delete(make_cache_path(path, name));
}
