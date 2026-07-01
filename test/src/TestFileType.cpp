// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Repository/FileType.hpp"
#include "DataFilePath.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "TestUtil.hpp"

#include <cstdint>
#include <cstdlib>

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
  ok1(FilenameMatchesFileType("london.openair", FileType::AIRSPACE));
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
  ok1(DetectFileTypeByFilename("london.openair") == FileType::AIRSPACE);
  ok1(DetectFileTypeByFilename("readme.md") == FileType::UNKNOWN);
}

static void
TestLayoutClassification()
{
  ok1(ClassifyDataFilename("waypoints.cup") == FileType::WAYPOINT);
  ok1(ClassifyDataFilename("track.igc") == FileType::IGC);
  ok1(GetLayoutSubdirForFilename("waypoints.cup") == Path("waypoints"));
  ok1(GetLayoutSubdirForFilename("zone.openair") == Path("airspace"));
  ok1(GetLayoutSubdirForFilename("task.tsk") == Path("tasks"));
  ok1(GetLayoutSubdirForFilename("gfs-rasp-forecast.dat") ==
      AllocatedPath::Build("weather", "rasp"));
  ok1(GetLayoutSubdirForFilename("track.igc") == Path("logs"));
  ok1(GetLayoutSubdirForFilename("readme.md") == nullptr);
  ok1(GetLayoutSubdirForFilename("profile.prf") == Path("profiles"));
  ok1(GetLayoutSubdirForFilename("plane.xcp") ==
      AllocatedPath::Build("profiles", "planes"));
  ok1(GetLayoutSubdirForFilename("repository") == Path("cache"));
  ok1(GetLayoutSubdirForFilename("xcsoar.log") == nullptr);
}

static void
TestRepositoryTypeStrings()
{
  ok1(FileTypeFromRepositoryString("airspace") == FileType::AIRSPACE);
  ok1(FileTypeFromRepositoryString("waypoint-details") ==
      FileType::WAYPOINTDETAILS);
  ok1(FileTypeFromRepositoryString("waypoint") == FileType::WAYPOINT);
  ok1(FileTypeFromRepositoryString("map") == FileType::MAP);
  ok1(FileTypeFromRepositoryString("flarmnet") == FileType::FLARMNET);
  ok1(FileTypeFromRepositoryString("rasp") == FileType::RASP);
  ok1(FileTypeFromRepositoryString("xci") == FileType::XCI);
  ok1(FileTypeFromRepositoryString("task") == FileType::TASK);
  ok1(FileTypeFromRepositoryString("checklist") == FileType::CHECKLIST);
  ok1(FileTypeFromRepositoryString("unknown") == FileType::UNKNOWN);
  ok1(FileTypeFromRepositoryString(nullptr) == FileType::UNKNOWN);
}

static void
TestDownloadRelativePath()
{
  ok1(GetFileTypeDownloadRelativePath(FileType::WAYPOINT, "test.cup") ==
      Path("waypoints/test.cup"));
  ok1(GetFileTypeDownloadRelativePath(FileType::WAYPOINT, "../evil.cup") ==
      nullptr);
  ok1(TypedDataSavePath(FileType::WAYPOINT, "../evil.cup") == nullptr);
}

static void
TestNestedDefaultDirs()
{
  ok1(GetFileTypeDefaultDir(FileType::RASP) ==
      AllocatedPath::Build("weather", "rasp"));
  ok1(GetFileTypeDefaultDir(FileType::WAYPOINTDETAILS) ==
      AllocatedPath::Build("waypoints", "details"));
  ok1(GetFileTypeDefaultDir(FileType::PLANE) ==
      AllocatedPath::Build("profiles", "planes"));
}

static void
TestEnsureFileTypeDownloadDirectory()
{
  char template_path[] = "/tmp/xcsoar-download-dir-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto rasp_dir = GetFileTypeDefaultDir(FileType::RASP);
  ok1(!Directory::Exists(LocalPath(rasp_dir)));

  ok1(EnsureFileTypeDownloadDirectory(FileType::RASP));
  ok1(Directory::Exists(LocalPath(rasp_dir)));

  ok1(GetFileTypeDownloadRelativePath(FileType::RASP, "forecast.dat") ==
      AllocatedPath::Build(Path(rasp_dir), Path("forecast.dat")));
}

int main()
{
  plan_tests(N_CONTENT_TYPES + 2 + N_CONTENT_TYPES + 2 + 7 + 16 + 9 + 12 +
             11 + 3 + 3 + 5);

  TestDefaultDirs();
  TestPatterns();
  TestSpecialFilenameType();
  TestFilenameMatchesFileType();
  TestDetectFileTypeByFilename();
  TestLayoutClassification();
  TestRepositoryTypeStrings();
  TestDownloadRelativePath();
  TestNestedDefaultDirs();
  TestEnsureFileTypeDownloadDirectory();

  return exit_status();
}
