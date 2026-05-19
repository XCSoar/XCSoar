// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TarBackup.hpp"

#include "CopyFile.hxx"
#include "FileReader.hxx"
#include "FileOutputStream.hxx"
#include "OutputStream.hxx"
#include "Reader.hxx"
#include "TarArchive.hpp"
#include "Operation/Operation.hpp"
#include "Language/Language.hpp"
#include "Storage/StorageDevice.hpp"
#include "Storage/StorageUtil.hpp"
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
  uint64_t size;
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

    items.push_back(ArchiveItem{AllocatedPath(full), name,
                                File::GetSize(full)});
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

std::vector<DirEntry>
EnumerateTarFiles(Path dir)
{
  std::vector<DirEntry> result;

  auto device = FindDeviceByName(dir);
  if (!device)
    return result;

  for (auto &entry : device->ListEntries(Path(""))) {
    if (entry.is_directory)
      continue;
    if (!Path(entry.name.c_str()).EndsWithIgnoreCase(".tar"))
      continue;

    result.push_back(std::move(entry));
  }

  return result;
}

bool
CreateBackup(Path source_root, OutputStream &output,
             ArchiveExcludePathFn exclude,
             OperationEnvironment &env,
             unsigned &created_files,
             std::string &error_message) noexcept
try {
  std::vector<ArchiveItem> items;
  if (!CollectSourceFiles(source_root, exclude, items, error_message))
    return false;

  uint64_t total_size = 0;
  for (const auto &item : items)
    total_size += item.size;

  /* Use kilobytes for progress to stay within unsigned range */
  const unsigned total_kb = static_cast<unsigned>(total_size / 1024);
  env.SetProgressRange(total_kb > 0 ? total_kb : 1);
  env.SetProgressPosition(0);

  created_files = 0;
  uint64_t bytes_done = 0;

  TarWriter writer(output);

  for (const auto &item : items) {
    env.SetText((std::string(_("Creating backup")) + "\n" +
                 item.archive_name).c_str());

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
    bytes_done += item.size;
    env.SetProgressPosition(static_cast<unsigned>(bytes_done / 1024));
  }

  writer.Finish();

  env.SetProgressPosition(total_kb > 0 ? total_kb : 1);
  return true;
} catch (const std::exception &e) {
  const char *w = e.what();
  error_message = (w != nullptr && ValidateUTF8(w))
    ? w : "Backup failed.";
  return false;
}

bool
RestoreBackup(Reader &input, Path destination_root,
              ArchiveExcludePathFn exclude,
              OperationEnvironment &env,
              unsigned &restored_files,
              unsigned &failed_files,
              std::string &error_message) noexcept
try {
  restored_files = 0;
  failed_files = 0;

  AllocatedPath temp_root =
    AllocatedPath::Build(destination_root, Path(".restore_tmp"));
  Directory::Create(temp_root);
  if (!Directory::Exists(temp_root)) {
    error_message = "Failed to create restore temp directory.";
    return false;
  }

  std::vector<std::string> extracted_names;

  /* Try to get total archive size for progress tracking */
  const uint64_t total_size = input.GetSize();
  const unsigned total_kb = static_cast<unsigned>(total_size / 1024);
  if (total_kb > 0) {
    env.SetProgressRange(total_kb);
    env.SetProgressPosition(0);
  }

  uint64_t bytes_done = 0;

  /* single pass: extract accepted entries to temp directory */
  try {
    TarReader reader(input);
    std::string name;
    uint64_t size;

    while (reader.Next(name, size)) {
      if (!AcceptEntry(exclude, name)) {
        reader.Skip();
        bytes_done += size;
        if (total_kb > 0)
          env.SetProgressPosition(static_cast<unsigned>(bytes_done / 1024));
        continue;
      }

      try {
        AllocatedPath dest = AllocatedPath::Build(temp_root, name.c_str());

        /* defense-in-depth: verify dest is under temp_root */
        if (dest.RelativeTo(temp_root) == nullptr) {
          reader.Skip();
          bytes_done += size;
          if (total_kb > 0)
            env.SetProgressPosition(static_cast<unsigned>(bytes_done / 1024));
          continue;
        }
        Directory::CreateRecursive(dest.GetParent());

        FileOutputStream out(dest, FileOutputStream::Mode::CREATE_VISIBLE);
        reader.ReadData(out);
        out.Commit();

        extracted_names.emplace_back(name);

        bytes_done += size;
        if (total_kb > 0)
          env.SetProgressPosition(static_cast<unsigned>(bytes_done / 1024));
      } catch (const std::exception &e) {
        throw std::runtime_error(std::string("Failed extracting: ") +
                                 name + ": " + e.what());
      } catch (...) {
        throw std::runtime_error(std::string("Failed extracting: ") +
                                 name);
      }
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
    Directory::CreateRecursive(dest.GetParent());

    try {
      MoveOrCopyFile(source, dest);
      ++restored_files;
    } catch (...) {
      ++failed_files;
    }
  }

  Directory::Remove(temp_root);

  if (total_kb > 0)
    env.SetProgressPosition(total_kb);
  return true;
} catch (const std::exception &e) {
  const char *w = e.what();
  error_message = (w != nullptr && ValidateUTF8(w))
    ? w : "Restore failed.";
  return false;
}
