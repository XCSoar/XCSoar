// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileType.hpp"
#include "system/Path.hpp"
#include "Compatibility/path.h"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"

#include <cstring>
#include <string_view>

/**
 * Per-type metadata: filename patterns, local subdirectory, and
 * optional repository index type string (type = …).
 */
struct FileTypeInfo {
  const char *patterns;
  const char *default_dir;
  const char *repository_type;
};

static constexpr FileTypeInfo file_type_info[unsigned(FileType::COUNT)] = {
  /* UNKNOWN */ {
    .patterns = "\0",
    .default_dir = nullptr,
    .repository_type = nullptr,
  },
  /* AIRSPACE */ {
    .patterns =
      "*.openair\0"
      "*.txt\0"
      "*.air\0"
      "*.sua\0",
    .default_dir = "airspace",
    .repository_type = "airspace",
  },
  /* RASP */ {
    .patterns = "*-rasp*.dat\0",
    .default_dir = "weather/rasp",
    .repository_type = "rasp",
  },
  /* WAYPOINT */ {
    .patterns =
      "*.dat\0"
      "*.xcw\0"
      "*.cup\0"
      "*.cupx\0"
      "*.wpz\0"
      "*.wpt\0",
    .default_dir = "waypoints",
    .repository_type = "waypoint",
  },
  /* WAYPOINTDETAILS */ {
    .patterns = "*.txt\0",
    .default_dir = "waypoints/details",
    .repository_type = "waypoint-details",
  },
  /* MAP */ {
    .patterns =
      "*.xcm\0"
      "*.lkm\0",
    .default_dir = "maps",
    .repository_type = "map",
  },
  /* FLARMNET */ {
    .patterns = "*.fln\0",
    .default_dir = "flarm",
    .repository_type = "flarmnet",
  },
  /* FLARMDB */ {
    .patterns =
      "xcsoar-flarm.txt\0"
      "flarm-msg-data.csv\0",
    .default_dir = "flarm",
    .repository_type = nullptr,
  },
  /* IGC */ {
    .patterns = "*.igc\0",
    .default_dir = "logs",
    .repository_type = nullptr,
  },
  /* NMEA */ {
    .patterns = "*.nmea\0",
    .default_dir = "logs",
    .repository_type = nullptr,
  },
  /* TASK */ {
    .patterns =
      "*.tsk\0"
      "*.cup\0"
      "*.igc\0",
    .default_dir = "tasks",
    .repository_type = "task",
  },
  /* CHECKLIST */ {
    .patterns =
      "*.xcc\0"
      "xcsoar-checklist.txt\0",
    .default_dir = "checklists",
    .repository_type = "checklist",
  },
  /* PROFILE */ {
    .patterns = "*.prf\0",
    .default_dir = "profiles",
    .repository_type = nullptr,
  },
  /* PLANE */ {
    .patterns = "*.xcp\0",
    .default_dir = "profiles/planes",
    .repository_type = nullptr,
  },
  /* XCI */ {
    .patterns = "*.xci\0",
    .default_dir = "input",
    .repository_type = "xci",
  },
  /* LUA */ {
    .patterns = "*.lua\0",
    .default_dir = "lua",
    .repository_type = nullptr,
  },
};

static_assert(ARRAY_SIZE(file_type_info) == unsigned(FileType::COUNT),
              "file_type_info must match FileType::COUNT");

static AllocatedPath
BuildDefaultDir(const char *dir) noexcept
{
  const char *slash = std::strchr(dir, '/');
#ifdef _WIN32
  if (slash == nullptr)
    slash = std::strchr(dir, '\\');
#endif
  if (slash == nullptr)
    return AllocatedPath(dir);

  AllocatedPath head(dir, slash);
  const char *tail = slash + 1;
  if (*tail == '\0')
    return head;

  return AllocatedPath::Build(Path(head), Path(BuildDefaultDir(tail)));
}

FileType
FileTypeFromRepositoryString(const char *type) noexcept
{
  if (type == nullptr || *type == '\0')
    return FileType::UNKNOWN;

  for (uint8_t i = 1; i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const auto &info = file_type_info[i];
    if (info.repository_type != nullptr &&
        StringIsEqual(type, info.repository_type))
      return static_cast<FileType>(i);
  }

  return FileType::UNKNOWN;
}

const char *
GetFileTypePatterns(const FileType file_type) noexcept
{
  if (file_type == FileType::UNKNOWN ||
      file_type == FileType::COUNT)
    return "\0";

  return file_type_info[unsigned(file_type)].patterns;
}

AllocatedPath
GetFileTypeDefaultDir(const FileType file_type)
{
  if (file_type == FileType::UNKNOWN ||
      file_type == FileType::COUNT)
    return nullptr;

  const char *dir = file_type_info[unsigned(file_type)].default_dir;
  return dir != nullptr ? BuildDefaultDir(dir) : nullptr;
}

FileType
SpecialFilenameType(const char *filename) noexcept
{
  for (uint8_t i = 1; i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const auto type = static_cast<FileType>(i);
    const char *p = GetFileTypePatterns(type);
    size_t length;
    while ((length = std::strlen(p)) > 0) {
      if (p[0] != '*' && StringIsEqualIgnoreCase(filename, p))
        return type;
      p += length + 1;
    }
  }

  return FileType::UNKNOWN;
}

bool
FilenameMatchesFileType(const char *filename,
                        FileType file_type) noexcept
{
  bool wildcard_match = false;

  const char *p = GetFileTypePatterns(file_type);
  size_t length;
  while ((length = std::strlen(p)) > 0) {
    if (p[0] == '*') {
      /* shell-style wildcard match: "*.lua" or "*-rasp*.dat" */
      if (WildcardMatchIgnoreCase(p, filename))
        wildcard_match = true;
    } else {
      /* exact match: "xcsoar-flarm.txt" — return immediately */
      if (StringIsEqualIgnoreCase(filename, p))
        return true;
    }
    p += length + 1;
  }

  if (!wildcard_match)
    return false;

  /* a wildcard matched, but check whether another type claims this
     filename with an exact pattern (e.g. "xcsoar-flarm.txt" should
     not be identified as an airspace file via "*.txt") */
  const auto special = SpecialFilenameType(filename);
  return special == FileType::UNKNOWN || special == file_type;
}

FileType
DetectFileTypeByFilename(const char *filename) noexcept
{
  if (filename == nullptr)
    return FileType::UNKNOWN;

  const auto special = SpecialFilenameType(filename);
  if (special != FileType::UNKNOWN)
    return special;

  FileType match = FileType::UNKNOWN;
  for (uint8_t i = 1; i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const auto type = static_cast<FileType>(i);
    if (!FilenameMatchesFileType(filename, type))
      continue;

    if (match != FileType::UNKNOWN) {
      if (match == FileType::WAYPOINT &&
          type == FileType::TASK &&
          Path(filename).EndsWithIgnoreCase(".cup"))
        return FileType::WAYPOINT;

      if (match == FileType::TASK &&
          type == FileType::WAYPOINT &&
          Path(filename).EndsWithIgnoreCase(".cup"))
        return FileType::WAYPOINT;

      return FileType::UNKNOWN;
    }

    match = type;
  }

  return match;
}

FileType
ClassifyDataFilename(const char *filename) noexcept
{
  const auto special = SpecialFilenameType(filename);
  if (special != FileType::UNKNOWN &&
      GetFileTypeDefaultDir(special) != nullptr)
    return special;

  for (uint8_t i = 1; i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const auto type = static_cast<FileType>(i);
    if (GetFileTypeDefaultDir(type) == nullptr)
      continue;

    if (FilenameMatchesFileType(filename, type))
      return type;
  }

  return FileType::UNKNOWN;
}

static bool
IsUserRepositoryFilename(const char *filename) noexcept
{
  static constexpr std::string_view prefix{"user_repository_"};
  return filename != nullptr &&
         StringStartsWith(filename, prefix);
}

AllocatedPath
GetLayoutSubdirForFilename(const char *filename) noexcept
{
  if (filename == nullptr || *filename == '\0')
    return nullptr;

  const FileType type = ClassifyDataFilename(filename);
  if (type != FileType::UNKNOWN) {
    AllocatedPath dir = GetFileTypeDefaultDir(type);
    if (dir != nullptr)
      return dir;
  }

  if (StringIsEqualIgnoreCase(filename, "notams.json") ||
      StringIsEqualIgnoreCase(filename, "repository") ||
      IsUserRepositoryFilename(filename))
    return AllocatedPath("cache");

  return nullptr;
}

bool
IsCacheLayoutFilename(const char *filename) noexcept
{
  const AllocatedPath subdir = GetLayoutSubdirForFilename(filename);
  return subdir != nullptr && subdir == AllocatedPath("cache");
}
