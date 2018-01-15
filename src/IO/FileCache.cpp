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

#include "FileCache.hpp"
#include "OS/FileUtil.hpp"
#include "Compiler.h"

#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef HAVE_POSIX
#include <windows.h>
#endif

static constexpr unsigned FILE_CACHE_MAGIC = 0xab352f8a;

#ifndef HAVE_POSIX

constexpr
static uint64_t
FileTimeToInteger(FILETIME ft)
{
  return ft.dwLowDateTime | ((uint64_t)ft.dwHighDateTime << 32);
}

#endif

struct FileInfo {
  uint64_t mtime;
  uint64_t size;

  bool operator==(const FileInfo &other) const {
    return mtime == other.mtime && size == other.size;
  }

  bool operator!=(const FileInfo &other) const {
    return !(*this == other);
  }

  /**
   * Is this date in the future?
   */
  gcc_pure
  bool IsFuture() const {
    return mtime > File::Now();
  }
};

gcc_pure
static inline bool
GetRegularFileInfo(Path path, FileInfo &info)
{
#ifdef HAVE_POSIX
  struct stat st;
  if (stat(path.c_str(), &st) < 0 || !S_ISREG(st.st_mode))
    return false;

  info.mtime = st.st_mtime;
  info.size = st.st_size;
  return true;
#else
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &data) ||
      (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return false;

  info.mtime = FileTimeToInteger(data.ftLastWriteTime);
  info.size = data.nFileSizeLow |
    ((uint64_t)data.nFileSizeHigh << 32);
  return true;
#endif
}

FileCache::FileCache(AllocatedPath &&_cache_path)
  :cache_path(std::move(_cache_path)) {}

void
FileCache::Flush(const TCHAR *name)
{
  File::Delete(MakeCachePath(name));
}

FILE *
FileCache::Load(const TCHAR *name, Path original_path)
{
  FileInfo original_info;
  if (!GetRegularFileInfo(original_path, original_info))
    return nullptr;

  const auto path = MakeCachePath(name);

  FileInfo cached_info;
  if (!GetRegularFileInfo(path, cached_info))
    return nullptr;

  /* if the original file is newer than the cache, discard the cache -
     unless the system clock is skewed (origina file's modification
     time is in the future) */
  if (original_info.mtime > cached_info.mtime && !original_info.IsFuture()) {
    File::Delete(path);
    return nullptr;
  }

  FILE *file = _tfopen(path.c_str(), _T("rb"));
  if (file == nullptr)
    return nullptr;

  unsigned magic;
  struct FileInfo old_info;
  if (fread(&magic, sizeof(magic), 1, file) != 1 ||
      magic != FILE_CACHE_MAGIC ||
      fread(&old_info, sizeof(old_info), 1, file) != 1 ||
      old_info != original_info) {
    fclose(file);
    File::Delete(path);
    return nullptr;
  }

  return file;
}

FILE *
FileCache::Save(const TCHAR *name, Path original_path)
{
  FileInfo original_info;
  if (!GetRegularFileInfo(original_path, original_info))
    return nullptr;

  Directory::Create(cache_path);

  const auto path = MakeCachePath(name);

  File::Delete(path);
  FILE *file = _tfopen(path.c_str(), _T("wb"));
  if (file == nullptr)
    return nullptr;

  if (fwrite(&FILE_CACHE_MAGIC, sizeof(FILE_CACHE_MAGIC), 1, file) != 1 ||
      fwrite(&original_info, sizeof(original_info), 1, file) != 1) {
    fclose(file);
    File::Delete(path);
    return nullptr;
  }

  return file;
}

bool
FileCache::Commit(const TCHAR *name, FILE *file)
{
  if (fclose(file) != 0) {
    File::Delete(MakeCachePath(name));
    return false;
  }

  return true;
}

void
FileCache::Cancel(const TCHAR *name, FILE *file)
{
  fclose(file);

  File::Delete(MakeCachePath(name));
}
