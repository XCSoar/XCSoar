// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ExportUtil.hpp"

#include "Form/DataField/MultiFile.hpp"
#include "Language/Language.hpp"
#include "system/FileUtil.hpp"
#include "util/ConvertString.hpp"
#include "Compatibility/path.h"

#include <cerrno>

static constexpr const char *kErrNoSpace = N_("No space left on device");
static constexpr const char *kErrNoWrite = N_("No write permission on target device");
static constexpr const char *kErrIo = N_("Device read/write error");

static void
TrimTrailingSeparators(std::string &path)
{
  while (path.size() > 1 && IsDirSeparator(path.back()))
    path.pop_back();
}

static void
TrimLeadingSeparators(std::string &path)
{
  while (!path.empty() && IsDirSeparator(path.front()))
    path.erase(path.begin());
}

AllocatedPath
BuildExportDirectory(Path target, std::string_view subfolder) noexcept
{
  if (target == nullptr)
    return AllocatedPath();

  std::string target_str(target.c_str());
  TrimTrailingSeparators(target_str);

  // std::string_view isn't null-terminated, create a temporary string
  std::string subfolder_str(subfolder);
  TrimLeadingSeparators(subfolder_str);
  TrimTrailingSeparators(subfolder_str);

  if (subfolder_str.empty())
    return AllocatedPath(target_str.c_str());

  return AllocatedPath::Build(Path(target_str.c_str()),
                              Path(subfolder_str.c_str()));
}

void
ScanFilesIntoDataField(const AllocatedPath &path, MultiFileDataField &df,
                       std::initializer_list<std::string_view> patterns,
                       bool recursive) noexcept
{
  (void)df.GetFileDataField().GetNumFiles();

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

  df.GetFileDataField().SortDesc();
}

std::string
ToUtf8String(const char *text) noexcept
{
  if (text == nullptr)
    return std::string();
  WideToUTF8Converter w(text);
  return w.IsValid() ? std::string(w.c_str()) : std::string();
}

std::string
MapErrnoToMessage(int err) noexcept
{
  if (err == ENOSPC)
    return ToUtf8String(gettext(kErrNoSpace));
  if (err == EACCES || err == EPERM || err == EROFS)
    return ToUtf8String(gettext(kErrNoWrite));
  if (err == EIO)
    return ToUtf8String(gettext(kErrIo));
  return std::string();
}
