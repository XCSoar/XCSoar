// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileTransferUtil.hpp"

#include "Form/DataField/MultiFile.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

AllocatedPath
BuildTargetDirectory(Path target, std::string_view subfolder) noexcept
{
  if (target == nullptr)
    return AllocatedPath();

  if (subfolder.empty())
    return AllocatedPath(target);

  return AllocatedPath::Build(target,
                              Path(std::string(subfolder).c_str()));
}

void
ScanFilesIntoDataField(const AllocatedPath &path, MultiFileDataField &df,
                       std::initializer_list<std::string_view> patterns,
                       bool recursive) noexcept
{
  struct FileScanner : File::Visitor {
    MultiFileDataField &df;
    explicit FileScanner(MultiFileDataField &d) : df(d) {}
    void Visit(Path file, Path /*filename*/) override {
      if (file == nullptr)
        return;
      df.GetFileDataField().ForceModify(file);
    }
  } scanner(df);

  for (const auto pattern : patterns) {
    if (pattern.empty())
      continue;
    // std::string_view isn't null-terminated, create a temporary string
    std::string pattern_str(pattern);
    Directory::VisitSpecificFiles(path, pattern_str.c_str(), scanner, recursive);
  }

  df.GetFileDataField().Sort(FileDataField::SortOrder::DESCENDING);
}
