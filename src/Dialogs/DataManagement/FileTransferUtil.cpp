// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileTransferUtil.hpp"

#include "Form/DataField/MultiFile.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <cstring>

namespace {

static void
VisitFileTypePatterns(Path path, File::Visitor &visitor,
                      std::initializer_list<FileType> file_types,
                      bool recursive) noexcept
{
  for (const auto file_type : file_types) {
    const char *patterns = GetFileTypePatterns(file_type);
    size_t length;
    while ((length = strlen(patterns)) > 0) {
      Directory::VisitSpecificFiles(path, patterns, visitor, recursive);
      patterns += length + 1;
    }
  }
}

} // namespace

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

void
ScanFilesIntoDataField(const AllocatedPath &path, MultiFileDataField &df,
                       std::initializer_list<FileType> file_types,
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

  VisitFileTypePatterns(path, scanner, file_types, recursive);

  df.GetFileDataField().Sort(FileDataField::SortOrder::DESCENDING);
}
