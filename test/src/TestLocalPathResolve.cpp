// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFilePath.hpp"
#include "DataFileLayout.hpp"
#include "Form/DataField/File.hpp"
#include "LocalPath.hpp"
#include "Repository/FileType.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "TestUtil.hpp"
#include "util/SpanCast.hxx"
#include "util/StringAPI.hxx"

#include <string_view>
#include <cstdio>
#include <stdlib.h>

static bool
TouchFile(const AllocatedPath &path) noexcept
{
  FileOutputStream out(path, FileOutputStream::Mode::CREATE);
  out.Commit();
  return true;
}

static bool
WriteTextFile(Path path, const char *content)
{
  FileOutputStream out(path, FileOutputStream::Mode::CREATE);
  out.Write(AsBytes(std::string_view(content)));
  out.Commit();
  return true;
}

static void
TestLayoutSubdirForFilename()
{
  ok1(GetLayoutSubdirForFilename("terrain.xcm") == Path("maps"));
  ok1(GetLayoutSubdirForFilename("zone.openair") == Path("airspace"));
  ok1(GetLayoutSubdirForFilename("user.cup") == Path("waypoints"));
  ok1(GetLayoutSubdirForFilename("profile.prf") == Path("profiles"));
  ok1(GetLayoutSubdirForFilename("plane.xcp") == Path("profiles/planes"));
  ok1(GetLayoutSubdirForFilename("repository") == Path("cache"));
  ok1(GetLayoutSubdirForFilename("user_repository_3") == Path("cache"));
  ok1(GetLayoutSubdirForFilename("notams.json") == Path("cache"));
  ok1(IsCacheLayoutFilename("repository"));
  ok1(IsCacheLayoutFilename("notams.json"));
  ok1(!IsCacheLayoutFilename("terrain.xcm"));
  ok1(GetLayoutSubdirForFilename("xcsoar.log") == nullptr);
  ok1(GetLayoutSubdirForFilename("flights.log") == nullptr);
  ok1(GetLayoutSubdirForFilename("foo.json") == nullptr);
}

static void
TestResolveTypedDataFilePath()
{
  char template_path[] = "/tmp/xcsoar-resolve-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto root = LocalPath("user.cup");
  const auto subdir = TypedDataSavePath(FileType::WAYPOINT, "user.cup");

  TouchFile(subdir);
  ok1(ResolveTypedDataFilePath(FileType::WAYPOINT, "user.cup") == subdir);

  File::Delete(subdir);
  TouchFile(root);
  ok1(ResolveTypedDataFilePath(FileType::WAYPOINT, "user.cup") == root);

  TouchFile(subdir);
  ok1(ResolveTypedDataFilePath(FileType::WAYPOINT, "user.cup") == subdir);
}

static void
TestResolveRepositoryDataPath()
{
  char template_path[] = "/tmp/xcsoar-repo-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto in_cache = CacheDataSavePath("repository");
  const auto in_data_cache =
    LocalPath(AllocatedPath::Build(Path("cache"), Path("repository")));
  const auto in_root = LocalPath("repository");

  TouchFile(in_cache);
  ok1(ResolveRepositoryDataPath("repository") == in_cache);

  File::Delete(in_cache);
  MakeLocalPath(Path("cache"));
  TouchFile(in_data_cache);
  ok1(ResolveRepositoryDataPath("repository") == in_data_cache);

  File::Delete(in_data_cache);
  TouchFile(in_root);
  ok1(ResolveRepositoryDataPath("repository") == in_root);

  File::Delete(in_root);
  MakeLocalPath(Path("repository"));
  const auto in_legacy =
    LocalPath(AllocatedPath::Build(Path("repository"), Path("repository")));
  TouchFile(in_legacy);
  ok1(ResolveRepositoryDataPath("repository") == in_legacy);
}

static void
TestResolveCacheDataPath()
{
  char template_path[] = "/tmp/xcsoar-notam-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto in_cache = CacheDataSavePath("notams.json");
  const auto in_data_cache =
    LocalPath(AllocatedPath::Build(Path("cache"), Path("notams.json")));
  const auto in_root = LocalPath("notams.json");

  TouchFile(in_cache);
  ok1(ResolveCacheDataPath("notams.json") == in_cache);

  File::Delete(in_cache);
  MakeLocalPath(Path("cache"));
  TouchFile(in_data_cache);
  ok1(ResolveCacheDataPath("notams.json") == in_data_cache);

  File::Delete(in_data_cache);
  TouchFile(in_root);
  ok1(ResolveCacheDataPath("notams.json") == in_root);
}

static void
TestResolveLogsDataPath()
{
  char template_path[] = "/tmp/xcsoar-logs-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto in_logs = LogsDataSavePath("xcsoar.log");
  const auto in_root = LocalPath("xcsoar.log");

  TouchFile(in_logs);
  ok1(ResolveLogsDataPath("xcsoar.log") == in_logs);

  File::Delete(in_logs);
  TouchFile(in_root);
  ok1(ResolveLogsDataPath("xcsoar.log") == in_root);
}

static void
TestDefaultTaskPaths()
{
  char template_path[] = "/tmp/xcsoar-task-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto save_path = TypedDataSavePath(FileType::TASK, "Default.tsk");
  ok1(StringFind(save_path.c_str(), "tasks") != nullptr);
  ok1(StringFind(save_path.c_str(), "Default.tsk") != nullptr);

  const auto legacy = LocalPath("Default.tsk");
  TouchFile(legacy);
  ok1(ResolveTypedDataFilePath(FileType::TASK, "Default.tsk") == legacy);

  File::Delete(legacy);
  TouchFile(save_path);
  ok1(ResolveTypedDataFilePath(FileType::TASK, "Default.tsk") == save_path);
}

static void
TestTypedDataSavePathCreatesNestedDirs()
{
  char template_path[] = "/tmp/xcsoar-typed-save-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto plane_path = TypedDataSavePath(FileType::PLANE, "plane.xcp");
  ok1(Directory::Exists(plane_path.GetParent()));

  const auto details_path =
    TypedDataSavePath(FileType::WAYPOINTDETAILS, "notes.txt");
  ok1(Directory::Exists(details_path.GetParent()));
}

static void
TestTextLayoutSniffing()
{
  char template_path[] = "/tmp/xcsoar-layout-sniff-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto airspace_file = LocalPath("known-airspace.txt");
  const auto tnp_file = LocalPath("known-tnp.txt");
  const auto single_ac_file = LocalPath("single-ac.txt");
  const auto bare_ac_file = LocalPath("bare-ac.txt");
  const auto details_file = LocalPath("known-details.txt");
  const auto unknown_file = LocalPath("unknown.txt");
  const auto log_file = LocalPath("xcsoar.log");

  ok1(WriteTextFile(airspace_file,
                    "AC C\nAN Test\nAL 1000 ft\nAH 2000 ft\n"));
  ok1(WriteTextFile(tnp_file, "TYPE=T\nTITLE=Test area\n"));
  ok1(WriteTextFile(single_ac_file, "AC C\nAN Test\nAL 1000 ft\n"));
  ok1(WriteTextFile(bare_ac_file, "AC\nAN Test\nAL 1000 ft\n"));
  ok1(WriteTextFile(details_file, "[HOME]\nimage=home.jpg\nRunway notes\n"));
  ok1(WriteTextFile(unknown_file, "[section]\nfoo=bar\n"));
  ok1(WriteTextFile(log_file, "log entry\n"));

  ok1(DataFileLayout::GetLayoutSubdirForDataFile(airspace_file) == Path("airspace"));
  ok1(DataFileLayout::GetLayoutSubdirForDataFile(tnp_file) == Path("airspace"));
  ok1(DataFileLayout::GetLayoutSubdirForDataFile(single_ac_file) == Path("airspace"));
  ok1(DataFileLayout::GetLayoutSubdirForDataFile(bare_ac_file) == nullptr);
  ok1(DataFileLayout::GetLayoutSubdirForDataFile(details_file) ==
      Path("waypoints/details"));
  ok1(DataFileLayout::GetLayoutSubdirForDataFile(unknown_file) == nullptr);
  ok1(DataFileLayout::GetLayoutSubdirForDataFile(log_file) == nullptr);

  const auto normalized_airspace =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(airspace_file);
  const auto normalized_tnp =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(tnp_file);
  const auto normalized_details =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(details_file);
  const auto normalized_unknown =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(unknown_file);
  const auto normalized_single_ac =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(single_ac_file);
  const auto normalized_bare_ac =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(bare_ac_file);
  const auto normalized_log =
    DataFileLayout::RelocateRootDataFileToLayoutSubdir(log_file);
  const auto expected_airspace =
    LocalPath(AllocatedPath::Build(Path("airspace"),
                                   Path("known-airspace.txt")));
  const auto expected_tnp =
    LocalPath(AllocatedPath::Build(Path("airspace"),
                                   Path("known-tnp.txt")));
  const auto expected_single_ac =
    LocalPath(AllocatedPath::Build(Path("airspace"),
                                   Path("single-ac.txt")));
  const auto expected_details = LocalPath(
    AllocatedPath::Build(AllocatedPath::Build(Path("waypoints"),
                                              Path("details")),
                         Path("known-details.txt")));

  ok1(normalized_airspace == expected_airspace);
  ok1(normalized_tnp == expected_tnp);
  ok1(normalized_details == expected_details);
  ok1(normalized_unknown == unknown_file);
  ok1(normalized_single_ac == expected_single_ac);
  ok1(normalized_bare_ac == bare_ac_file);
  ok1(normalized_log == log_file);
  ok1(File::Exists(normalized_airspace));
  ok1(File::Exists(normalized_tnp));
  ok1(File::Exists(normalized_details));
  ok1(File::Exists(normalized_unknown));
  ok1(File::Exists(normalized_single_ac));
  ok1(File::Exists(normalized_log));
  ok1(!File::Exists(airspace_file));
  ok1(!File::Exists(tnp_file));
  ok1(!File::Exists(single_ac_file));
  ok1(File::Exists(bare_ac_file));
  ok1(!File::Exists(details_file));
}

static void
TestWaypointDetailsPickerFindsLegacyAirspaceTxt()
{
  char template_path[] = "/tmp/xcsoar-details-picker-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto airspace_dir = LocalPath(Path("airspace"));
  const auto details_dir = LocalPath(AllocatedPath::Build(Path("waypoints"),
                                                          Path("details")));
  Directory::CreateRecursive(airspace_dir);
  Directory::CreateRecursive(details_dir);
  ok1(Directory::Exists(airspace_dir));
  ok1(Directory::Exists(details_dir));

  const auto root_file = LocalPath("root-details.txt");
  const auto details_file =
    AllocatedPath::Build(details_dir, Path("typed-details.txt"));
  const auto legacy_airspace_file =
    AllocatedPath::Build(airspace_dir, Path("legacy-details.txt"));
  TouchFile(root_file);
  TouchFile(details_file);
  TouchFile(legacy_airspace_file);

  FileDataField df;
  df.SetFileType(FileType::WAYPOINTDETAILS);
  df.ScanMultiplePatterns(GetFileTypePatterns(FileType::WAYPOINTDETAILS));
  ok1(df.GetNumFiles() >= 3);

  ok1(df.Find(root_file) >= 0);
  ok1(df.Find(details_file) >= 0);
  ok1(df.Find(legacy_airspace_file) >= 0);
}

static void
TestRepositoryDownloadRelativePath()
{
  const auto relative = RepositoryDownloadRelativePath("repository");
  ok1(StringFind(relative.c_str(), "cache") != nullptr);
  ok1(StringFind(relative.c_str(), "repository") != nullptr);
}

int
main()
{
  plan_tests(80);
  TestLayoutSubdirForFilename();
  TestResolveTypedDataFilePath();
  TestResolveRepositoryDataPath();
  TestResolveCacheDataPath();
  TestResolveLogsDataPath();
  TestDefaultTaskPaths();
  TestTypedDataSavePathCreatesNestedDirs();
  TestTextLayoutSniffing();
  TestWaypointDetailsPickerFindsLegacyAirspaceTxt();
  TestRepositoryDownloadRelativePath();
  return exit_status();
}
