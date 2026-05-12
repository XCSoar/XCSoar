// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightCache.hpp"

#include "system/FileUtil.hpp"

#include <chrono>

namespace {

class OlderThanFileVisitor final : public File::Visitor {
  const std::chrono::system_clock::time_point cutoff;

public:
  explicit OlderThanFileVisitor(std::chrono::system_clock::time_point _cutoff) noexcept
    :cutoff(_cutoff) {}

  void Visit(Path full_path, [[maybe_unused]] Path filename) override {
    if (File::GetLastModification(full_path) < cutoff)
      File::Delete(full_path);
  }
};

} // namespace

namespace SkysightCache {

void
Cleanup(Path directory) noexcept
{
  const auto now = std::chrono::system_clock::now();
  OlderThanFileVisitor delete_tiles{now - std::chrono::hours{12}};
  OlderThanFileVisitor delete_tmp{now - std::chrono::hours{6}};
  OlderThanFileVisitor delete_json{now - std::chrono::hours{1}};

  Directory::VisitSpecificFiles(directory, "*.jpg", delete_tiles);
  Directory::VisitSpecificFiles(directory, "*.tmp", delete_tmp);
  Directory::VisitSpecificFiles(directory, "*.json", delete_json);
}

} // namespace SkysightCache
