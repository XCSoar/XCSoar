// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "TestUtil.hpp"

#include <cstdint>

static constexpr auto N_CONTENT_TYPES =
  static_cast<uint8_t>(FileType::COUNT) - 1;

static void
TestDefaultDirs()
{
  // Every content type must return either nullptr or a valid
  // relative path (never absolute, never empty)
  for (uint8_t i = 1;
       i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    auto dir = GetFileTypeDefaultDir(static_cast<FileType>(i));
    if (dir != nullptr) {
      ok(!dir.empty() && !dir.IsAbsolute(),
         "FileType %u dir is relative", unsigned(i));
    } else {
      ok(true, "FileType %u has no dir", unsigned(i));
    }
  }

  // Sentinel types return nullptr
  ok1(GetFileTypeDefaultDir(FileType::UNKNOWN) == nullptr);
  ok1(GetFileTypeDefaultDir(FileType::COUNT) == nullptr);
}

static void
TestPatterns()
{
  // Every content type should have at least one non-empty pattern
  for (uint8_t i = 1;
       i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const char *p = GetFileTypePatterns(static_cast<FileType>(i));
    ok(p != nullptr && p[0] != '\0',
       "FileType %u has patterns", unsigned(i));
  }

  // Sentinel types return empty pattern
  const char *p = GetFileTypePatterns(FileType::UNKNOWN);
  ok1(p != nullptr && p[0] == '\0');

  p = GetFileTypePatterns(FileType::COUNT);
  ok1(p != nullptr && p[0] == '\0');
}

static void
TestSpecialFilenameType()
{
  // Exact-match patterns should return their type
  ok1(SpecialFilenameType("xcsoar-flarm.txt") == FileType::FLARMDB);
  ok1(SpecialFilenameType("flarm-msg-data.csv") == FileType::FLARMDB);
  ok1(SpecialFilenameType("xcsoar-checklist.txt") == FileType::CHECKLIST);

  // Case-insensitive
  ok1(SpecialFilenameType("XCSoar-Flarm.TXT") == FileType::FLARMDB);

  // Wildcard-only patterns should not match
  ok1(SpecialFilenameType("test.lua") == FileType::UNKNOWN);
  ok1(SpecialFilenameType("airspace.txt") == FileType::UNKNOWN);
  ok1(SpecialFilenameType("nonexistent.xyz") == FileType::UNKNOWN);
}

static void
TestFilenameMatchesFileType()
{
  // Basic wildcard matches
  ok1(FilenameMatchesFileType("init.lua", FileType::LUA));
  ok1(FilenameMatchesFileType("test.igc", FileType::IGC));
  ok1(FilenameMatchesFileType("map.xcm", FileType::MAP));
  ok1(FilenameMatchesFileType("profile.prf", FileType::PROFILE));
  ok1(FilenameMatchesFileType("gfs-rasp-forecast.dat", FileType::RASP));

  // Exact-match patterns
  ok1(FilenameMatchesFileType("xcsoar-flarm.txt", FileType::FLARMDB));
  ok1(FilenameMatchesFileType("xcsoar-checklist.txt", FileType::CHECKLIST));

  // Key test: wildcard should NOT match if another type has an exact
  // claim on this filename (xcsoar-flarm.txt matches *.txt for
  // AIRSPACE, but FLARMDB owns it exactly)
  ok1(!FilenameMatchesFileType("xcsoar-flarm.txt", FileType::AIRSPACE));
  ok1(!FilenameMatchesFileType("xcsoar-checklist.txt", FileType::AIRSPACE));

  // Regular .txt file should still match AIRSPACE (no exact claim)
  ok1(FilenameMatchesFileType("london.txt", FileType::AIRSPACE));
  ok1(FilenameMatchesFileType("gfs-rasp-forecast.dat", FileType::WAYPOINT));

  // No match at all
  ok1(!FilenameMatchesFileType("readme.md", FileType::LUA));
  ok1(!FilenameMatchesFileType("photo.jpg", FileType::MAP));

  // Case-insensitive
  ok1(FilenameMatchesFileType("INIT.LUA", FileType::LUA));
  ok1(FilenameMatchesFileType("XCSoar-Flarm.TXT", FileType::FLARMDB));
}

static void
TestDetectFileTypeByFilename()
{
  ok1(DetectFileTypeByFilename("xcsoar-flarm.txt") == FileType::FLARMDB);
  ok1(DetectFileTypeByFilename("track.igc") == FileType::UNKNOWN);
  ok1(DetectFileTypeByFilename("xcsoar-rasp-eu.dat") == FileType::UNKNOWN);
  ok1(DetectFileTypeByFilename("profile.prf") == FileType::PROFILE);
  ok1(DetectFileTypeByFilename("waypoints.cup") == FileType::WAYPOINT);
  ok1(DetectFileTypeByFilename("WAYPOINTS.CUP") == FileType::WAYPOINT);
  ok1(DetectFileTypeByFilename("london.txt") == FileType::UNKNOWN);
  ok1(DetectFileTypeByFilename("readme.md") == FileType::UNKNOWN);
}

static void
TestLayoutClassification()
{
  ok1(ClassifyDataFilename("waypoints.cup") == FileType::WAYPOINT);
  ok1(ClassifyDataFilename("track.igc") == FileType::IGC);
  ok1(GetLayoutSubdirForFilename("waypoints.cup") == Path("waypoints"));
  ok1(GetLayoutSubdirForFilename("task.tsk") == Path("tasks"));
  ok1(GetLayoutSubdirForFilename("gfs-rasp-forecast.dat") == Path("weather/rasp"));
  ok1(GetLayoutSubdirForFilename("track.igc") == Path("logs"));
  ok1(GetLayoutSubdirForFilename("readme.md") == nullptr);
  ok1(GetLayoutSubdirForFilename("profile.prf") == Path("profiles"));
  ok1(GetLayoutSubdirForFilename("plane.xcp") == Path("profiles/planes"));
  ok1(GetLayoutSubdirForFilename("repository") == Path("cache"));
  ok1(GetLayoutSubdirForFilename("xcsoar.log") == nullptr);
}

int main()
{
  plan_tests(N_CONTENT_TYPES + 2 + N_CONTENT_TYPES + 2 + 7 + 15 + 8 + 11);

  TestDefaultDirs();
  TestPatterns();
  TestSpecialFilenameType();
  TestFilenameMatchesFileType();
  TestDetectFileTypeByFilename();
  TestLayoutClassification();

  return exit_status();
}
