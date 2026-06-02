// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataLayoutMigration.hpp"

#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "UtilsSettings.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "util/IterableSplitString.hxx"

#include <string>
#include <vector>

static constexpr char MIGRATION_MARKER[] = ".xcsoar-subdir-layout-v1";

MigrationEntry::MigrationEntry(AllocatedPath &&_source_path,
                               AllocatedPath &&_destination_path,
                               FileType _type) noexcept
  : source_path(std::move(_source_path)),
    destination_path(std::move(_destination_path)),
    type(_type) {}

MigrationPlan
BuildMigrationPlan(Path root)
{
  struct Visitor final : Directory::DirEntryVisitor {
    Path root;
    MigrationPlan &plan;

    Visitor(Path _root, MigrationPlan &_plan) noexcept
      : root(_root), plan(_plan) {}

    void Visit(Path full, Path filename, bool is_dir) noexcept override {
      if (is_dir)
        return;

      if (!filename.IsValidFilename() || filename == Path(MIGRATION_MARKER))
        return;

      const auto relative_path = RelativePath(full);
      if (relative_path == nullptr || !relative_path.IsBase())
        return;

      const auto subdir = GetLayoutSubdirForFilename(filename.c_str());
      if (subdir == nullptr) {
        ++plan.skipped_unknown;
        return;
      }

      auto destination_path = LocalPath(AllocatedPath::Build(subdir, filename));
      if (File::ExistsAny(destination_path)) {
        ++plan.skipped_conflicts;
        return;
      }

      plan.moves.emplace_back(AllocatedPath(full),
                              std::move(destination_path),
                              DetectFileTypeByFilename(filename.c_str()));
    }
  };

  MigrationPlan plan;
  Visitor visitor(root, plan);
  Directory::VisitDirectoriesAndFiles(root, visitor, false);
  return plan;
}

static bool
UpdateSingleProfilePath(std::string_view key, Path old_path, Path new_path)
{
  std::string value;
  if (!Profile::Get(key, value) || value.empty())
    return false;

  const auto expanded = ExpandLocalPath(Path(value.c_str()));
  if (expanded == nullptr || expanded != old_path)
    return false;

  Profile::SetPath(key, new_path);
  return true;
}

static bool
UpdateMultipleProfilePaths(std::string_view key, Path old_path, Path new_path)
{
  std::string value;
  if (!Profile::Get(key, value) || value.empty())
    return false;

  const auto contracted_new_path = ContractLocalPath(new_path);
  const char *new_value = contracted_new_path != nullptr
    ? contracted_new_path.c_str()
    : new_path.c_str();

  bool changed = false;
  std::string updated_value;
  for (const auto part : TIterableSplitString(value.c_str(), '|')) {
    if (!updated_value.empty())
      updated_value.push_back('|');

    const std::string current(part);
    const auto expanded = ExpandLocalPath(Path(current.c_str()));
    if (expanded != nullptr && expanded == old_path) {
      updated_value.append(new_value);
      changed = true;
    } else {
      updated_value.append(current);
    }
  }

  if (changed)
    Profile::Set(key, updated_value);

  return changed;
}

bool
UpdateProfileReferences(Path old_path, Path new_path)
{
  bool changed = false;

  bool key_changed = UpdateSingleProfilePath(ProfileKeys::MapFile,
                                             old_path, new_path);
  changed |= key_changed;
  MapFileChanged |= key_changed;

  key_changed = UpdateMultipleProfilePaths(ProfileKeys::WaypointFileList,
                                           old_path, new_path);
  key_changed |= UpdateMultipleProfilePaths(ProfileKeys::WatchedWaypointFileList,
                                            old_path, new_path);
  changed |= key_changed;
  WaypointFileChanged |= key_changed;

  key_changed = UpdateMultipleProfilePaths(ProfileKeys::AirfieldFileList,
                                           old_path, new_path);
  changed |= key_changed;
  AirfieldFileChanged |= key_changed;

  key_changed = UpdateMultipleProfilePaths(ProfileKeys::AirspaceFileList,
                                           old_path, new_path);
  changed |= key_changed;
  AirspaceFileChanged |= key_changed;

  key_changed = UpdateSingleProfilePath(ProfileKeys::FlarmFile,
                                        old_path, new_path);
  changed |= key_changed;
  FlarmFileChanged |= key_changed;

  key_changed = UpdateSingleProfilePath(ProfileKeys::RaspFile,
                                        old_path, new_path);
  changed |= key_changed;
  RaspFileChanged |= key_changed;

  key_changed = UpdateSingleProfilePath(ProfileKeys::ChecklistFile,
                                        old_path, new_path);
  changed |= key_changed;
  ChecklistFileChanged |= key_changed;

  key_changed = UpdateSingleProfilePath(ProfileKeys::InputFile,
                                        old_path, new_path);
  changed |= key_changed;
  InputFileChanged |= key_changed;

  changed |= UpdateSingleProfilePath("PlanePath", old_path, new_path);

  if (Profile::GetPath() == old_path) {
    Profile::SetFiles(new_path);
    changed = true;
  }

  return changed;
}

void
MigrateDataLayoutToSubdirs() noexcept
{
  try {
    const auto root = GetPrimaryDataPath();
    const auto marker = LocalPath(MIGRATION_MARKER);
    if (File::Exists(marker))
      return;

    const auto plan = BuildMigrationPlan(root);

    bool profile_changed = false;
    unsigned moved_count = 0;
    for (const auto &entry : plan.moves) {
      const auto parent = entry.destination_path.GetParent();
      if (parent != nullptr)
        Directory::CreateRecursive(parent);

      if (!File::Rename(entry.source_path, entry.destination_path))
        continue;

      ++moved_count;
      profile_changed |= UpdateProfileReferences(entry.source_path,
                                                  entry.destination_path);
    }

    if (profile_changed)
      Profile::Save();

    if (plan.moves.empty() || moved_count > 0) {
      FileOutputStream marker_file(marker, FileOutputStream::Mode::CREATE);
      marker_file.Commit();
    } else if (!plan.moves.empty()) {
      LogFormat("Data layout migration: all %u file moves failed",
                unsigned(plan.moves.size()));
    }
  } catch (...) {
    LogError(std::current_exception());
  }
}
