// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFilePath.hpp"

#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <utility>

namespace {

[[gnu::pure]]
static AllocatedPath
ResolveDataCachePath(const char *filename) noexcept
{
  return filename != nullptr
    ? LocalPath(AllocatedPath::Build(Path("cache"), Path(filename)))
    : nullptr;
}

[[gnu::pure]]
static AllocatedPath
ResolveUniqueExistingTypedDataFile(const char *filename) noexcept
{
  if (filename == nullptr || *filename == '\0')
    return nullptr;

  AllocatedPath match;

  for (uint8_t i = 1; i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const auto type = static_cast<FileType>(i);
    const auto subdir = GetFileTypeDefaultDir(type);
    if (subdir == nullptr || !FilenameMatchesFileType(filename, type))
      continue;

    auto candidate = LocalPath(AllocatedPath::Build(subdir, Path(filename)));
    if (!File::Exists(candidate))
      continue;

    if (match != nullptr && match != candidate)
      return nullptr;

    match = std::move(candidate);
  }

  return match;
}

} // namespace

AllocatedPath
TypedDataSavePath(FileType file_type, const char *filename) noexcept
{
  const auto subdir = GetFileTypeDefaultDir(file_type);
  if (subdir == nullptr || filename == nullptr || *filename == '\0')
    return filename != nullptr ? LocalPath(filename) : nullptr;

  Directory::CreateRecursive(LocalPath(subdir));
  return LocalPath(AllocatedPath::Build(subdir, filename));
}

AllocatedPath
ResolveTypedDataFilePath(FileType file_type, const char *filename) noexcept
{
  return ResolveTypedDataFilePath(file_type, filename, {});
}

AllocatedPath
ResolveTypedDataFilePath(FileType file_type, const char *filename,
                         std::initializer_list<const char *> legacy_names) noexcept
{
  if (filename == nullptr || *filename == '\0')
    return nullptr;

  auto typed = TypedDataSavePath(file_type, filename);
  if (typed != nullptr && File::Exists(typed))
    return typed;

  auto legacy_root = LocalPath(filename);
  if (File::Exists(legacy_root))
    return legacy_root;

  for (const char *legacy_name : legacy_names) {
    if (legacy_name == nullptr || *legacy_name == '\0')
      continue;

    auto legacy = LocalPath(legacy_name);
    if (File::Exists(legacy))
      return legacy;
  }

  return typed;
}

AllocatedPath
ResolveLocalDataFile(AllocatedPath path, FileType file_type) noexcept
{
  if (path == nullptr)
    return nullptr;

  if (File::Exists(path))
    return path;

  const auto relative = RelativePath(path);
  if (relative == nullptr)
    return path;

  const auto filename = relative.GetBase();
  if (filename == nullptr || !filename.IsValidFilename())
    return path;

  if (file_type == FileType::UNKNOWN) {
    if (auto resolved = ResolveUniqueExistingTypedDataFile(filename.c_str());
        resolved != nullptr)
      return resolved;

    file_type = ClassifyDataFilename(filename.c_str());
  }

  if (file_type == FileType::UNKNOWN)
    return path;

  auto resolved = ResolveTypedDataFilePath(file_type, filename.c_str());
  if (resolved != nullptr)
    return resolved;

  return path;
}

AllocatedPath
RepositoryDataSavePath(const char *filename) noexcept
{
  Directory::CreateRecursive(GetCachePath());
  return AllocatedPath::Build(GetCachePath(), filename);
}

AllocatedPath
ResolveRepositoryDataPath(const char *filename) noexcept
{
  auto in_cache = RepositoryDataSavePath(filename);
  if (File::Exists(in_cache))
    return in_cache;

  auto in_data_cache = ResolveDataCachePath(filename);
  if (File::Exists(in_data_cache))
    return in_data_cache;

  auto in_root = LocalPath(filename);
  if (File::Exists(in_root))
    return in_root;

  auto in_legacy = LocalPath(AllocatedPath::Build(Path("repository"),
                                                  Path(filename)));
  if (File::Exists(in_legacy))
    return in_legacy;

  return in_cache;
}

AllocatedPath
RepositoryDownloadRelativePath(const char *filename) noexcept
{
  return AllocatedPath::Build(Path("cache"), Path(filename));
}

AllocatedPath
ResolveDownloadDestinationPath(Path path) noexcept
{
  if (path == nullptr)
    return nullptr;

  return path.IsAbsolute() ? AllocatedPath(path) : LocalPath(path);
}

AllocatedPath
CacheDataSavePath(const char *filename) noexcept
{
  return RepositoryDataSavePath(filename);
}

AllocatedPath
ResolveCacheDataPath(const char *filename) noexcept
{
  auto in_cache = CacheDataSavePath(filename);
  if (File::Exists(in_cache))
    return in_cache;

  auto in_data_cache = ResolveDataCachePath(filename);
  if (File::Exists(in_data_cache))
    return in_data_cache;

  auto in_root = LocalPath(filename);
  if (File::Exists(in_root))
    return in_root;

  return in_cache;
}

AllocatedPath
LogsDataSavePath(const char *filename) noexcept
{
  const auto logs_dir = LocalPath(Path("logs"));
  Directory::CreateRecursive(logs_dir);
  return AllocatedPath::Build(logs_dir, filename);
}

AllocatedPath
ResolveLogsDataPath(const char *filename) noexcept
{
  auto in_logs = LogsDataSavePath(filename);
  if (File::Exists(in_logs))
    return in_logs;

  auto in_root = LocalPath(filename);
  if (File::Exists(in_root))
    return in_root;

  return in_logs;
}
