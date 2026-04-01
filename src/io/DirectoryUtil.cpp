// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DirectoryUtil.hpp"

#include "system/FileUtil.hpp"

#include <utility>

namespace DirectoryUtil {

class LambdaVisitor final : public File::Visitor {
  std::function<void(Path, Path)> visitor;

public:
  explicit LambdaVisitor(std::function<void(Path, Path)> &&_visitor) noexcept
    :visitor(std::move(_visitor)) {}

  void Visit(Path path, Path filename) override
  {
    visitor(path, filename);
  }
};

void
VisitSpecificFiles(Path directory, const char *pattern,
                   std::function<void(Path, Path)> visitor)
{
  LambdaVisitor v(std::move(visitor));
  Directory::VisitSpecificFiles(directory, pattern, v);
}

unsigned
DeleteSpecificFilesExcept(Path directory, const char *pattern,
                          std::function<bool(Path, Path)> keep)
{
  unsigned deleted = 0;

  VisitSpecificFiles(directory, pattern,
                     [&deleted, &keep](Path path, Path filename) {
                       if (keep(path, filename))
                         return;

                       if (File::Delete(path))
                         ++deleted;
                     });

  return deleted;
}

} // namespace DirectoryUtil

