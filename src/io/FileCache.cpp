// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileCache.hpp"
#include "FileReader.hxx"
#include "FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "util/SpanCast.hxx"

#ifdef _WIN32
#include "time/FileTime.hxx"
#endif

#include <cstdint>
#include <stdexcept>

#include <string.h>

#ifdef HAVE_POSIX
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <unistd.h>
#else
#   include <fileapi.h>
#endif

static constexpr unsigned FILE_CACHE_MAGIC = 0xab352f8b;

struct FileInfo {
  std::chrono::system_clock::time_point mtime;
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
  [[gnu::pure]]
  bool IsFuture() const {
    return mtime > std::chrono::system_clock::now();
  }
};

static inline bool
GetRegularFileInfo(Path path, FileInfo &info)
{
#ifdef HAVE_POSIX
  struct stat st;
  if (stat(path.c_str(), &st) < 0 || !S_ISREG(st.st_mode))
    return false;

  info.mtime = std::chrono::system_clock::from_time_t(st.st_mtime);
  info.size = st.st_size;
  return true;
#else
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &data) ||
      (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return false;

  info.mtime = FileTimeToChrono(data.ftLastWriteTime);
  info.size = data.nFileSizeLow |
    ((uint64_t)data.nFileSizeHigh << 32);
  return true;
#endif
}

FileCache::FileCache(AllocatedPath &&_cache_path)
  :cache_path(std::move(_cache_path)) {}

void
FileCache::Flush(const char *name)
{
  File::Delete(MakeCachePath(name));
}

std::unique_ptr<Reader>
FileCache::Load(const char *name, Path original_path) noexcept
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

  try {
    auto r = std::make_unique<FileReader>(path);

    unsigned magic;
    struct FileInfo old_info;

    r->ReadT(magic);
    r->ReadT(old_info);

    if (magic == FILE_CACHE_MAGIC &&
        old_info == original_info)
      return r;
  } catch (...) {
  }

  File::Delete(path);
  return nullptr;
}

std::unique_ptr<FileOutputStream>
FileCache::Save(const char *name, Path original_path)
{
  FileInfo original_info;
  if (!GetRegularFileInfo(original_path, original_info))
    // TODO: proper error message (GetRegularFileInfo() should throw)
    throw std::runtime_error("Cannot access cached file");

  Directory::Create(cache_path);

  const auto path = MakeCachePath(name);

  File::Delete(path);

  auto os = std::make_unique<FileOutputStream>(path);
  os->Write(ReferenceAsBytes(FILE_CACHE_MAGIC));
  os->Write(ReferenceAsBytes(original_info));
  return os;
}
