// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DirEntry.hpp"
#include "system/FileUtil.hpp"

#include <chrono>

std::vector<DirEntry>
ListDirEntries(Path dir)
{
  std::vector<DirEntry> result;

  struct Visitor : public Directory::DirEntryVisitor {
    std::vector<DirEntry> &out;
    explicit Visitor(std::vector<DirEntry> &o) noexcept : out(o) {}

    void Visit(Path path, Path filename, bool is_dir) noexcept override {
      DirEntry e;
      e.name = filename.c_str();
      e.is_directory = is_dir;

      if (!is_dir) {
        const uint64_t sz = File::GetSize(path);
        if (sz > 0)
          e.size = sz;

        const auto mtime = File::GetLastModification(path);
        if (mtime != std::chrono::system_clock::time_point{})
          e.last_modified_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
              mtime.time_since_epoch()).count();
      }

      out.push_back(std::move(e));
    }
  } visitor(result);

  Directory::VisitDirectoriesAndFiles(dir, visitor);
  return result;
}
