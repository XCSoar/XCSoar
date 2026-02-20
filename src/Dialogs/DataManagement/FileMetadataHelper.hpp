// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/FileUtil.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <string>
#include <unordered_map>
#include <vector>

struct FileMetadataEntry {
  StaticString<32u> size;
  StaticString<32u> last_modified;
};

class FileMetadataHelper {
  std::unordered_map<std::string, FileMetadataEntry> entries;

public:
  void Build(const std::vector<Path> &paths) noexcept {
    entries.clear();
    entries.reserve(paths.size());

    for (const auto &path : paths) {
      if (path == nullptr)
        continue;

      FileMetadataEntry item;
      if (File::Exists(path)) {
        FormatByteSize(item.size.buffer(), item.size.capacity(),
                       File::GetSize(path));
        FormatISO8601(item.last_modified.buffer(),
                      BrokenDateTime{File::GetLastModification(path)});
      } else {
        item.size.clear();
        item.last_modified.clear();
      }

      entries.emplace(path.ToUTF8(), std::move(item));
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

  const char *GetLastModifiedText(Path path) const noexcept {
    const auto *entry = Find(path);
    return entry != nullptr ? entry->last_modified.c_str() : nullptr;
  }
};
