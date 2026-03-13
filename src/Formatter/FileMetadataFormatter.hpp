// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/DirEntry.hpp"
#include "system/FileUtil.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

struct FileMetadataEntry {
  StaticString<32u> size;
  StaticString<32u> last_modified;
  std::optional<uint64_t> raw_size;
};

class FileMetadataFormatter {
  std::unordered_map<std::string, FileMetadataEntry> entries;

public:
  /**
   * Format a single DirEntry's metadata into a FileMetadataEntry.
   */
  static FileMetadataEntry FormatEntry(const DirEntry &de) noexcept {
    FileMetadataEntry item;
    item.raw_size = de.size;

    if (de.size)
      FormatByteSize(item.size.buffer(), item.size.capacity(),
                     *de.size);
    else
      item.size.clear();

    if (de.last_modified_ms) {
      const auto tp = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(*de.last_modified_ms));
      FormatISO8601(item.last_modified.buffer(), BrokenDateTime{tp});
    } else {
      item.last_modified.clear();
    }

    return item;
  }
  void Build(const std::vector<Path> &paths) noexcept {
    entries.clear();
    entries.reserve(paths.size());

    for (const auto &path : paths) {
      if (path == nullptr)
        continue;

      FileMetadataEntry item;
      if (File::Exists(path)) {
        uint64_t file_size = File::GetSize(path);
        item.raw_size = file_size;
        FormatByteSize(item.size.buffer(), item.size.capacity(),
                       file_size);
        FormatISO8601(item.last_modified.buffer(),
                      BrokenDateTime{File::GetLastModification(path)});
      } else {
        item.size.clear();
        item.last_modified.clear();
      }

      entries.emplace(path.ToUTF8(), std::move(item));
    }
  }

  /**
   * Build metadata from DirEntry items already carrying size and
   * last-modified information (avoids re-statting the filesystem).
   *
   * @param dir_entries directory listing with metadata
   * @param base_dir   the directory the entries belong to; each
   *                   entry's key is base_dir/entry.name
   */
  void Build(const std::vector<DirEntry> &dir_entries,
             Path base_dir) noexcept {
    entries.clear();
    entries.reserve(dir_entries.size());

    for (const auto &de : dir_entries) {
      if (de.is_directory)
        continue;

      auto item = FormatEntry(de);

      AllocatedPath full = AllocatedPath::Build(base_dir, de.name.c_str());
      entries.emplace(full.ToUTF8(), std::move(item));
    }
  }

  const FileMetadataEntry *Find(Path path) const noexcept {
    if (path == nullptr)
      return nullptr;

    const auto found = entries.find(path.ToUTF8());
    return found != entries.end() ? &found->second : nullptr;
  }

  const char *GetSizeText(Path path) const noexcept {
    const auto *entry = Find(path);
    return entry != nullptr ? entry->size.c_str() : nullptr;
  }

  std::optional<uint64_t> GetRawSize(Path path) const noexcept {
    const auto *entry = Find(path);
    return entry != nullptr ? entry->raw_size : std::nullopt;
  }

  const char *GetLastModifiedText(Path path) const noexcept {
    const auto *entry = Find(path);
    return entry != nullptr ? entry->last_modified.c_str() : nullptr;
  }
};
