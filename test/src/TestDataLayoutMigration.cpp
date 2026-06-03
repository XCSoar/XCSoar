// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataLayoutMigration.hpp"
#include "FakeLogFile.hpp"
#include "LocalPath.hpp"
#include "Profile/Current.hpp"
#include "Profile/File.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Map.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "io/FileOutputStream.hxx"
#include "io/OutputStream.hxx"
#include "system/FileUtil.hpp"
#include "TestUtil.hpp"
#include "util/SpanCast.hxx"
#include "util/StringAPI.hxx"

#include <string_view>

#include <cstdio>
#include <stdlib.h>
#include <sys/stat.h>

static bool
WriteTextFile(Path path, const char *content) noexcept
{
  FileOutputStream out(path, FileOutputStream::Mode::CREATE);
  out.Write(AsBytes(std::string_view(content)));
  out.Commit();
  return true;
}

static void
TestMigratesFilesAndActiveProfileOnly()
{
  char template_path[] = "/tmp/xcsoar-migrate-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);

  const Path data_path(template_path);
  SetSingleDataPath(data_path);

  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("terrain.xcm")),
                    "dummy"));
  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("plane.xcp")),
                    "plane"));
  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("repository")),
                    "name=test\n"));
  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("notams.json")),
                    "{}\n"));

  const auto default_prf =
    AllocatedPath::Build(data_path, Path("default.prf"));
  const auto competition_prf =
    AllocatedPath::Build(data_path, Path("competition.prf"));
  ok1(WriteTextFile(default_prf,
                    "MapFile=%LOCAL_PATH%\\terrain.xcm\n"
                    "PlanePath=%LOCAL_PATH%\\plane.xcp\n"));
  ok1(WriteTextFile(competition_prf,
                    "MapFile=%LOCAL_PATH%\\terrain.xcm\n"
                    "PlanePath=%LOCAL_PATH%\\plane.xcp\n"));

  Profile::SetFiles(default_prf);
  Profile::Load();

  MigrateDataLayoutToSubdirs();

  const auto map_path =
    AllocatedPath::Build(AllocatedPath::Build(data_path, Path("maps")),
                         Path("terrain.xcm"));
  ok1(File::Exists(map_path));
  ok1(!File::Exists(AllocatedPath::Build(data_path, Path("terrain.xcm"))));

  const auto active_map = Profile::GetPath(ProfileKeys::MapFile);
  ok1(active_map != nullptr);
  ok1(StringFind(active_map.c_str(), "maps") != nullptr);

  std::string raw_active_map;
  ok1(Profile::Get(ProfileKeys::MapFile, raw_active_map));
  ok1(StringFind(raw_active_map.c_str(), "maps") != nullptr);

  const auto active_plane = Profile::GetPath("PlanePath");
  ok1(active_plane != nullptr);
  ok1(StringFind(active_plane.c_str(), "profiles") != nullptr);
  ok1(StringFind(active_plane.c_str(), "planes") != nullptr);

  std::string raw_active_plane;
  ok1(Profile::Get("PlanePath", raw_active_plane));
  ok1(StringFind(raw_active_plane.c_str(), "profiles") != nullptr);
  ok1(StringFind(raw_active_plane.c_str(), "planes") != nullptr);

  ok1(Profile::GetPath() != nullptr);
  ok1(StringFind(Profile::GetPath().c_str(), "profiles") != nullptr);

  const auto moved_competition_prf =
    AllocatedPath::Build(AllocatedPath::Build(data_path, Path("profiles")),
                         Path("competition.prf"));
  ok1(File::Exists(moved_competition_prf));
  ok1(!File::Exists(competition_prf));

  ProfileMap competition_map;
  Profile::LoadFile(competition_map, moved_competition_prf);
  std::string raw_map_path;
  ok1(competition_map.Get(ProfileKeys::MapFile, raw_map_path));
  ok1(StringFind(raw_map_path.c_str(), "maps") == nullptr);

  const auto resolved = competition_map.GetPath(ProfileKeys::MapFile);
  ok1(resolved != nullptr);
  ok1(File::Exists(resolved));
  ok1(StringFind(resolved.c_str(), "maps") != nullptr);

  ok1(File::Exists(LocalPath(".xcsoar-subdir-layout-v1")));
  ok1(File::Exists(AllocatedPath::Build(AllocatedPath::Build(data_path,
                                                             Path("cache")),
                                        Path("repository"))));
  ok1(!File::Exists(AllocatedPath::Build(data_path, Path("repository"))));
  ok1(File::Exists(AllocatedPath::Build(AllocatedPath::Build(data_path,
                                                             Path("cache")),
                                        Path("notams.json"))));
  ok1(!File::Exists(AllocatedPath::Build(data_path, Path("notams.json"))));
  ok1(File::Exists(AllocatedPath::Build(AllocatedPath::Build(data_path,
                                                             Path("profiles")),
                                        Path("default.prf"))));
  ok1(File::Exists(AllocatedPath::Build(AllocatedPath::Build(AllocatedPath::Build(data_path,
                                                                                  Path("profiles")),
                                                             Path("planes")),
                                        Path("plane.xcp"))));
  ok1(!File::Exists(AllocatedPath::Build(data_path, Path("plane.xcp"))));
}

static void
TestDoesNotWriteMarkerWhenAllMovesFail()
{
  char template_path[] = "/tmp/xcsoar-migrate-fail-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);

  const Path data_path(template_path);
  SetSingleDataPath(data_path);

  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("terrain.xcm")),
                    "dummy"));

  const auto default_prf =
    AllocatedPath::Build(data_path, Path("default.prf"));
  ok1(WriteTextFile(default_prf,
                    "MapFile=%LOCAL_PATH%\\terrain.xcm\n"));

  Profile::SetFiles(default_prf);
  Profile::Load();

  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("maps")),
                    "not a directory"));
  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("profiles")),
                    "not a directory"));

  MigrateDataLayoutToSubdirs();

  ok1(!File::Exists(LocalPath(".xcsoar-subdir-layout-v1")));
  ok1(File::Exists(AllocatedPath::Build(data_path, Path("terrain.xcm"))));
}

int
main()
{
  SetFakeLogFileQuiet(true);

  plan_tests(43);
  TestMigratesFilesAndActiveProfileOnly();
  TestDoesNotWriteMarkerWhenAllMovesFail();
  return exit_status();
}
