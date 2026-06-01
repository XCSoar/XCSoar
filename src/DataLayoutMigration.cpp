// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataLayoutMigration.hpp"

#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Profile/Current.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Map.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/IterableSplitString.hxx"
#include "util/StringCompare.hxx"

#include <string>
#include <vector>

#include <windef.h>

static constexpr char MIGRATION_MARKER[] = ".xcsoar-subdir-layout-v1";

struct PathMove {
  AllocatedPath old_path;
  AllocatedPath new_path;
};

static constexpr std::string_view profile_path_keys[] = {
  ProfileKeys::MapFile,
  ProfileKeys::WaypointFileList,
  ProfileKeys::WatchedWaypointFileList,
  ProfileKeys::AirfieldFileList,
  ProfileKeys::AirspaceFileList,
  ProfileKeys::FlarmFile,
  ProfileKeys::RaspFile,
  ProfileKeys::ChecklistFile,
  ProfileKeys::LanguageFile,
  ProfileKeys::InputFile,
  ProfileKeys::PolarFile,
  "PlanePath",
};

[[gnu::pure]]
static bool
IsMigrationMarker(Path filename) noexcept
{
  return filename == Path(MIGRATION_MARKER);
}

static bool
MoveFileToSubdir(const PathMove &move) noexcept
{
  if (File::Exists(move.new_path)) {
    if (File::Exists(move.old_path))
      LogFmt("Data layout migration: skip {}, already at {}",
             move.old_path, move.new_path);
    return false;
  }

  if (!File::Exists(move.old_path))
    return false;

  const auto parent = move.new_path.GetParent();
  if (parent != nullptr && !parent.empty())
    Directory::Create(parent);

  if (File::Rename(move.old_path, move.new_path))
    return true;

  LogFmt("Data layout migration: failed to move {} to {}",
         move.old_path, move.new_path);
  return false;
}

static bool
RewritePathValue(ProfileMap &map, std::string_view key,
                 const std::vector<PathMove> &moves) noexcept
{
  const auto current = map.GetPath(key);
  if (current == nullptr)
    return false;

  for (const auto &move : moves) {
    if (current == move.old_path) {
      map.SetPath(key, move.new_path);
      return true;
    }
  }

  return false;
}

static bool
RewriteMultiPathValue(ProfileMap &map, std::string_view key,
                      const std::vector<PathMove> &moves) noexcept
{
  char buffer[MAX_PATH * 4];
  if (!map.Get(key, std::span{buffer}))
    return false;

  if (StringIsEmpty(buffer))
    return false;

  bool changed = false;
  std::string result;

  for (const auto segment : TIterableSplitString(buffer, '|')) {
    if (segment.empty())
      continue;

    const std::string segment_string(segment);
    const Path segment_path(segment_string.c_str());
    AllocatedPath expanded = ExpandLocalPath(segment_path);
    if (expanded == nullptr)
      expanded = AllocatedPath(segment_path);

    bool segment_changed = false;
    for (const auto &move : moves) {
      if (expanded == move.old_path) {
        if (!result.empty())
          result += '|';
        const auto contracted = ContractLocalPath(move.new_path);
        result += contracted != nullptr
          ? contracted.c_str()
          : move.new_path.c_str();
        changed = true;
        segment_changed = true;
        break;
      }
    }

    if (!segment_changed) {
      if (!result.empty())
        result += '|';
      result += segment_string;
    }
  }

  if (!changed)
    return false;

  map.Set(key, result.c_str());
  return true;
}

static bool
RewriteActiveProfile(const std::vector<PathMove> &moves) noexcept
{
  if (moves.empty())
    return false;

  bool changed = false;

  for (const auto key : profile_path_keys) {
    if (key == ProfileKeys::WaypointFileList ||
        key == ProfileKeys::WatchedWaypointFileList ||
        key == ProfileKeys::AirfieldFileList ||
        key == ProfileKeys::AirspaceFileList)
      changed |= RewriteMultiPathValue(Profile::map, key, moves);
    else
      changed |= RewritePathValue(Profile::map, key, moves);
  }

  return changed;
}

class DataRootFileCollector final : public File::Visitor {
public:
  std::vector<Path> filenames;

  void Visit(Path path, Path filename) override {
    (void)path;

    if (IsMigrationMarker(filename))
      return;

    if (!filename.IsValidFilename())
      return;

    filenames.emplace_back(filename);
  }
};

#ifdef ANDROID
static void
TryMigrateCacheFile(const PathMove &move,
                    std::vector<PathMove> &moves) noexcept
{
  if (move.old_path == move.new_path)
    return;

  if (MoveFileToSubdir(move))
    moves.push_back(move);
  else if (File::Exists(move.new_path) && !File::Exists(move.old_path))
    moves.push_back(move);
}

static void
MigrateCacheFilesFromDirectory(const AllocatedPath &dir,
                               std::vector<PathMove> &moves) noexcept
{
  if (!Directory::Exists(dir))
    return;

  DataRootFileCollector collector;
  Directory::VisitFiles(dir, collector, false);

  for (const Path filename : collector.filenames) {
    const char *name = filename.c_str();
    if (!IsCacheLayoutFilename(name))
      continue;

    PathMove move;
    move.old_path = AllocatedPath::Build(dir, filename);
    move.new_path = CacheDataSavePath(name);
    TryMigrateCacheFile(move, moves);
  }
}
#endif

static void
MigrateDataRoot(Path data_path, std::vector<PathMove> &all_moves) noexcept
{
  const auto marker = AllocatedPath::Build(data_path, Path(MIGRATION_MARKER));
  if (File::Exists(marker)) {
    LogDebug("Data layout migration: already done for {}", data_path);
    return;
  }

  DataRootFileCollector collector;
  Directory::VisitFiles(data_path, collector, false);

  std::vector<PathMove> moves;
  moves.reserve(collector.filenames.size());

  for (const Path filename : collector.filenames) {
    const char *name = filename.c_str();

#ifdef ANDROID
    if (IsCacheLayoutFilename(name)) {
      PathMove move;
      move.old_path = AllocatedPath::Build(data_path, filename);
      move.new_path = CacheDataSavePath(name);
      TryMigrateCacheFile(move, moves);
      continue;
    }
#endif

    const AllocatedPath subdir = GetLayoutSubdirForFilename(name);
    if (subdir == nullptr)
      continue;

    PathMove move;
    move.old_path = AllocatedPath::Build(data_path, filename);
    move.new_path =
      AllocatedPath::Build(AllocatedPath::Build(data_path, subdir),
                           filename);

    if (move.old_path == move.new_path)
      continue;

    if (MoveFileToSubdir(move))
      moves.push_back(std::move(move));
    else if (File::Exists(move.new_path) && !File::Exists(move.old_path))
      moves.push_back(std::move(move));
  }

#ifdef ANDROID
  MigrateCacheFilesFromDirectory(
    AllocatedPath::Build(data_path, Path("cache")), moves);
  MigrateCacheFilesFromDirectory(
    AllocatedPath::Build(data_path, Path("repository")), moves);
#endif

  all_moves.insert(all_moves.end(),
                   std::make_move_iterator(moves.begin()),
                   std::make_move_iterator(moves.end()));

  {
    FileOutputStream marker_file(marker, FileOutputStream::Mode::CREATE);
    marker_file.Commit();
  }

  LogFmt("Data layout migration: completed for {}", data_path);
}

static void
MigrateDataRootCallback(Path data_path, void *ctx) noexcept
{
  auto &all_moves = *static_cast<std::vector<PathMove> *>(ctx);
  MigrateDataRoot(data_path, all_moves);
}

void
MigrateDataLayoutToSubdirs() noexcept
{
  std::vector<PathMove> all_moves;

  try {
    VisitDataPaths(MigrateDataRootCallback, &all_moves);

    if (RewriteActiveProfile(all_moves)) {
      Profile::Save();
      LogFmt("Data layout migration: updated active profile {}",
             Profile::GetPath());
    }
  } catch (...) {
    LogError(std::current_exception(), "Data layout migration failed");
  }
}
