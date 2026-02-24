// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TarBackup.hpp"

#include "CopyFile.hxx"
#include "Formatter/ByteSizeFormatter.hpp"
#include "FileReader.hxx"
#include "FileOutputStream.hxx"
#include "OutputStream.hxx"
#include "Reader.hxx"
#include "TarArchive.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/Operation.hpp"
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

constexpr uint64_t TAR_HEADER_SIZE = 512;
constexpr uint64_t TAR_END_MARKER_SIZE = 2 * TAR_HEADER_SIZE;
constexpr uint64_t BACKUP_SYNC_INTERVAL = 4u * 1024u * 1024u;

[[nodiscard]]
uint64_t
PadTarBlockSize(uint64_t size) noexcept
{
  return (TAR_HEADER_SIZE - (size % TAR_HEADER_SIZE)) % TAR_HEADER_SIZE;
}

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

[[nodiscard]]
uint64_t
CalculateArchiveSize(const std::vector<ArchiveItem> &items) noexcept
{
  uint64_t total = TAR_END_MARKER_SIZE;

  for (const auto &item : items)
    total += TAR_HEADER_SIZE + item.size + PadTarBlockSize(item.size);

  return total;
}

class TransferProgress {
  OperationEnvironment &env;
  const uint64_t total_size;
  const unsigned progress_range;

  std::string item_name;
  uint64_t bytes_done = 0;

public:
  TransferProgress(OperationEnvironment &_env, const char *action,
                   uint64_t _total_size, unsigned _progress_range) noexcept
    : env(_env), total_size(_total_size), progress_range(_progress_range) {
    SetMessage(action);
  }

  void SetItem(std::string_view name)
  {
    item_name.assign(name);
    SetMessage(item_name.c_str());
  }

  void SetBytesDone(uint64_t value) noexcept
  {
    bytes_done = std::min(value, total_size);

    const unsigned position = bytes_done >= total_size
      ? progress_range
      : std::min(progress_range,
                 static_cast<unsigned>(bytes_done / 1024));

    env.SetProgressPosition(position);
    SetMessage(item_name.c_str());
  }

  void Advance(uint64_t delta) noexcept
  {
    SetBytesDone(bytes_done + delta);
  }

  void Complete() noexcept
  {
    SetBytesDone(total_size);
  }

private:
  void SetMessage(const char *headline) noexcept
  {
    char done_buffer[32], total_buffer[32];
    FormatByteSize(done_buffer, sizeof(done_buffer), bytes_done);
    FormatByteSize(total_buffer, sizeof(total_buffer), total_size);

    const std::string progress = std::string(done_buffer) + " / " + total_buffer;
    if (headline == nullptr || *headline == '\0')
      env.SetText(progress.c_str());
    else
      env.SetText((std::string(headline) + "\n" + progress).c_str());
  }
};

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

  const uint64_t total_size = CalculateArchiveSize(items);

  /* Use kilobytes for progress to stay within unsigned range */
  const unsigned progress_range = static_cast<unsigned>(total_size / 1024) > 0
    ? static_cast<unsigned>(total_size / 1024)
    : 1;
  env.SetProgressRange(progress_range);
  env.SetProgressPosition(0);
  TransferProgress progress(env, _("Creating backup"), total_size,
                            progress_range);

  auto *file_output = dynamic_cast<FileOutputStream *>(&output);
  uint64_t synced_position = 0;
  uint64_t next_sync_position = BACKUP_SYNC_INTERVAL;

  const auto sync_progress = [&](bool force) {
    if (file_output == nullptr)
      return;

    const uint64_t current_position = file_output->Tell();
    if (!force && current_position < next_sync_position)
      return;

    if (current_position == synced_position)
      return;

    file_output->Sync();
    synced_position = current_position;
    next_sync_position = synced_position + BACKUP_SYNC_INTERVAL;
    progress.SetBytesDone(synced_position);
  };

  created_files = 0;

  TarWriter writer(output);

  for (const auto &item : items) {
    if (env.IsCancelled())
      throw OperationCancelled{};

    progress.SetItem(item.archive_name);

    if (file_output == nullptr)
      progress.Advance(TAR_HEADER_SIZE);

    try {
      FileReader in(item.source_path);
      writer.Add(item.archive_name, in, in.GetSize(),
                 [&env, &progress, &sync_progress, file_output](uint64_t delta) {
                   if (env.IsCancelled())
                     throw OperationCancelled{};

                   if (file_output != nullptr)
                     sync_progress(false);
                   else
                     progress.Advance(delta);
                 });
    } catch (const OperationCancelled &) {
      throw;
    } catch (const std::exception &e) {
      throw std::runtime_error(std::string("Failed adding file: ") +
                               item.archive_name + ": " + e.what());
    } catch (...) {
      throw std::runtime_error(std::string("Failed adding file: ") +
                               item.archive_name);
    }

    ++created_files;

    if (file_output == nullptr)
      progress.Advance(PadTarBlockSize(item.size));
  }

  progress.SetItem(_("Finalizing backup"));
  writer.Finish();

  if (file_output != nullptr)
    sync_progress(true);
  else
    progress.Advance(TAR_END_MARKER_SIZE);

  progress.Complete();
  return true;
} catch (const OperationCancelled &) {
  error_message = _("Backup cancelled.");
  return false;
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

  AllocatedPath temp_root = AllocatedPath(destination_root) + ".restore_tmp";
  Directory::Create(temp_root);
  if (!Directory::Exists(temp_root)) {
    error_message = "Failed to create restore temp directory.";
    return false;
  }

  std::vector<std::string> extracted_names;

  /* Track archive input bytes for smooth restore progress. */
  const uint64_t total_size = input.GetSize();
  const unsigned progress_range = static_cast<unsigned>(total_size / 1024) > 0
    ? static_cast<unsigned>(total_size / 1024)
    : 1;
  env.SetProgressRange(progress_range);
  env.SetProgressPosition(0);
  TransferProgress progress(env, _("Restoring backup"), total_size,
                            progress_range);

  /* single pass: extract accepted entries to temp directory */
  try {
    TarReader reader(input);
    std::string name;
    uint64_t size;

    while (reader.Next(name, size)) {
      if (env.IsCancelled())
        throw OperationCancelled{};

      progress.Advance(TAR_HEADER_SIZE);
      progress.SetItem(name);

      if (!AcceptEntry(exclude, name)) {
        reader.Skip([&env, &progress](uint64_t delta) {
          if (env.IsCancelled())
            throw OperationCancelled{};
          progress.Advance(delta);
        });
        progress.Advance(PadTarBlockSize(size));
        continue;
      }

      try {
        AllocatedPath dest = AllocatedPath::Build(temp_root, name.c_str());

        /* defense-in-depth: verify dest is under temp_root */
        if (dest.RelativeTo(temp_root) == nullptr) {
          reader.Skip([&env, &progress](uint64_t delta) {
            if (env.IsCancelled())
              throw OperationCancelled{};
            progress.Advance(delta);
          });
          progress.Advance(PadTarBlockSize(size));
          continue;
        }
        Directory::CreateRecursive(dest.GetParent());

        FileOutputStream out(dest, FileOutputStream::Mode::CREATE_VISIBLE);
        reader.ReadData(out, [&env, &progress](uint64_t delta) {
          if (env.IsCancelled())
            throw OperationCancelled{};
          progress.Advance(delta);
        });
        out.Commit();
        progress.Advance(PadTarBlockSize(size));

        extracted_names.emplace_back(name);
      } catch (const OperationCancelled &) {
        throw;
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
    if (env.IsCancelled()) {
      Directory::Remove(temp_root);
      throw OperationCancelled{};
    }

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

  progress.Complete();
  return true;
} catch (const OperationCancelled &) {
  error_message = _("Restore cancelled.");
  return false;
} catch (const std::exception &e) {
  const char *w = e.what();
  error_message = (w != nullptr && ValidateUTF8(w))
    ? w : "Restore failed.";
  return false;
}
