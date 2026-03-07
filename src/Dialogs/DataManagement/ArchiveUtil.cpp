// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ArchiveUtil.hpp"

#include "io/CopyFile.hxx"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "io/TarArchive.hpp"
#include "Operation/Operation.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/UTF8.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

namespace {

struct ArchiveItem {
  AllocatedPath source_path;
  std::string archive_name;
};

[[nodiscard]] std::string
MakeArchiveName(Path full, Path root)
{
  Path rel = full.RelativeTo(root);
  if (rel != nullptr) {
    std::string name = rel.c_str();
    std::replace(name.begin(), name.end(), '\\', '/');
    return name;
  }

  /* fallback: return bare filename */
  Path base = full.GetBase();
  return base != nullptr ? base.c_str() : full.c_str();
}

void
EnsureParentDirExists(Path path)
{
  AllocatedPath parent = path.GetParent();
  if (parent == nullptr || Directory::Exists(parent))
    return;

  EnsureParentDirExists(parent);
  Directory::Create(parent);
}

class CollectVisitor final : public File::Visitor {
  ArchiveExcludePathFn exclude;
  Path root;
  std::vector<ArchiveItem> &items;

public:
  CollectVisitor(ArchiveExcludePathFn exclude_, Path root_,
                 std::vector<ArchiveItem> &items_) noexcept
    : exclude(exclude_), root(root_), items(items_) {}

  void
  Visit(Path full, Path /*filename*/) override
  {
    const std::string name = MakeArchiveName(full, root);
    if (exclude != nullptr && exclude(name))
      return;

    items.push_back(ArchiveItem{AllocatedPath(full), name});
  }
};

[[nodiscard]]
bool
CollectSourceFiles(Path source_root, ArchiveExcludePathFn exclude,
                   std::vector<ArchiveItem> &items,
                   std::string &error_message) noexcept
try {
  items.clear();
  CollectVisitor visitor(exclude, source_root, items);
  Directory::VisitFiles(source_root, visitor, true);
  return true;
} catch (const std::exception &e) {
  const char *w = e.what();
  error_message = (w != nullptr && ValidateUTF8(w))
    ? w : "Operation failed.";
  return false;
}

[[nodiscard]]
bool
AcceptEntry(ArchiveExcludePathFn exclude, std::string_view name) noexcept
{
  return !name.empty() && (exclude == nullptr || !exclude(name));
}

} // namespace

bool
CreateBackup(Path source_root, Path destination_tar,
             ArchiveExcludePathFn exclude,
             OperationEnvironment &env,
             unsigned &created_files,
             std::string &error_message) noexcept
try {
  std::vector<ArchiveItem> items;
  if (!CollectSourceFiles(source_root, exclude, items, error_message))
    return false;

  env.SetProgressRange(items.size() + 1);
  env.SetProgressPosition(0);

  created_files = 0;

  FileOutputStream fos(destination_tar,
                        FileOutputStream::Mode::CREATE_VISIBLE);
  TarWriter writer(fos);

  for (const auto &item : items) {
    try {
      FileReader in(item.source_path);
      writer.Add(item.archive_name, in, in.GetSize());
    } catch (const std::exception &e) {
      throw std::runtime_error(std::string("Failed adding file: ") +
                               item.archive_name + ": " + e.what());
    } catch (...) {
      throw std::runtime_error(std::string("Failed adding file: ") +
                               item.archive_name);
    }

    ++created_files;
    env.SetProgressPosition(created_files);

    /* flush to disk after each file so that progress reflects
       actual I/O and the final Commit() does not stall */
    fos.Sync();
  }

  writer.Finish();
  fos.Commit();

  env.SetProgressPosition(items.size() + 1);
  return true;
} catch (const std::exception &e) {
  const char *w = e.what();
  error_message = (w != nullptr && ValidateUTF8(w))
    ? w : "Backup failed.";
  return false;
}

bool
RestoreBackup(Path tar_file, Path destination_root,
              ArchiveExcludePathFn exclude,
              OperationEnvironment &env,
              unsigned &restored_files,
              unsigned &failed_files,
              std::string &error_message) noexcept
try {
  /* first pass: count accepted entries for progress bar */
  unsigned total_entries = 0;
  {
    FileReader in(tar_file);
    TarReader reader(in);
    std::string name;
    uint64_t size;

    while (reader.Next(name, size)) {
      if (AcceptEntry(exclude, name))
        ++total_entries;
      reader.Skip();
    }
  }

  /* progress covers extract + move phases */
  const unsigned progress_range = total_entries * 2;
  env.SetProgressRange(progress_range);
  env.SetProgressPosition(0);

  restored_files = 0;
  failed_files = 0;

  AllocatedPath temp_root = AllocatedPath(destination_root) + ".restore_tmp";
  Directory::Create(temp_root);
  if (!Directory::Exists(temp_root)) {
    error_message = "Failed to create restore temp directory.";
    return false;
  }

  std::vector<std::string> extracted_names;
  unsigned progress = 0;

  /* second pass: extract accepted entries to temp directory */
  try {
    FileReader in(tar_file);
    TarReader reader(in);
    std::string name;
    uint64_t size;

    while (reader.Next(name, size)) {
      if (!AcceptEntry(exclude, name)) {
        reader.Skip();
        continue;
      }

      try {
        AllocatedPath dest = AllocatedPath::Build(temp_root, name.c_str());
        EnsureParentDirExists(dest);

        FileOutputStream out(dest, FileOutputStream::Mode::CREATE_VISIBLE);
        reader.ReadData(out);
        out.Commit();

        extracted_names.emplace_back(name);
      } catch (const std::exception &e) {
        throw std::runtime_error(std::string("Failed extracting: ") +
                                 name + ": " + e.what());
      } catch (...) {
        throw std::runtime_error(std::string("Failed extracting: ") +
                                 name);
      }

      env.SetProgressPosition(++progress);
    }
  } catch (...) {
    failed_files = 1;
    Directory::Remove(temp_root);
    throw;
  }

  /* move extracted files to final destination */
  for (const auto &name : extracted_names) {
    AllocatedPath source = AllocatedPath::Build(temp_root, name.c_str());
    AllocatedPath dest = AllocatedPath::Build(destination_root, name.c_str());
    EnsureParentDirExists(dest);

    try {
      MoveOrCopyFile(source, dest);
      ++restored_files;
    } catch (...) {
      ++failed_files;
    }

    env.SetProgressPosition(++progress);
  }

  Directory::Remove(temp_root);

  env.SetProgressPosition(progress_range);
  return true;
} catch (const std::exception &e) {
  const char *w = e.what();
  error_message = (w != nullptr && ValidateUTF8(w))
    ? w : "Restore failed.";
  return false;
}
