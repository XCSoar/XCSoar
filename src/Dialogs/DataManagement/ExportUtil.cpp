// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ExportUtil.hpp"

#include "Form/DataField/MultiFile.hpp"
#include "Language/Language.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <cerrno>

static constexpr const char *kExportErrorMessages[] = {
  N_("No space left on device"),
  N_("No write permission on target device"),
  N_("Device read/write error"),
  N_("Unknown error"),
};

static_assert(std::size(kExportErrorMessages) == static_cast<size_t>(ExportError::COUNT),
              "kExportErrorMessages size must match ExportError::COUNT");

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

const char *
GetErrorMessage(ExportError err) noexcept
{
  const auto index = static_cast<unsigned>(err);
  if (index >= std::size(kExportErrorMessages))
    return nullptr;
  return kExportErrorMessages[index];
}

std::string
MapErrnoToError(int err) noexcept
{
  if (err == 0)
    return std::string();

  ExportError kind = ExportError::Unknown;
  if (err == ENOSPC)
    kind = ExportError::NoSpace;
  else if (err == EACCES || err == EPERM || err == EROFS)
    kind = ExportError::NoWrite;
  else if (err == EIO)
    kind = ExportError::Io;

  const char *msg = GetErrorMessage(kind);
  if (msg == nullptr)
    return std::string();
  return gettext(msg);
}
