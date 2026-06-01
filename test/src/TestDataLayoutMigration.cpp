// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataLayoutMigration.hpp"
#include "TestUtil.hpp"
#include "LocalPath.hpp"
#include "Profile/File.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Map.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Current.hpp"
#include "io/FileOutputStream.hxx"
#include "io/OutputStream.hxx"
#include "system/FileUtil.hpp"
#include "util/SpanCast.hxx"
#include "util/StringAPI.hxx"

#include <string_view>

#include <cstdio>

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
  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("repository")),
                     "name=test\n"));
  ok1(WriteTextFile(AllocatedPath::Build(data_path, Path("notams.json")),
                     "{}\n"));
  const auto default_prf =
    AllocatedPath::Build(data_path, Path("default.prf"));
  const auto competition_prf =
    AllocatedPath::Build(data_path, Path("competition.prf"));
  ok1(WriteTextFile(default_prf,
                     "MapFile=%LOCAL_PATH%\\terrain.xcm\n"));
  ok1(WriteTextFile(competition_prf,
                     "MapFile=%LOCAL_PATH%\\terrain.xcm\n"));

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

  ProfileMap competition_map;
  Profile::LoadFile(competition_map, competition_prf);
  const auto stored =
    ExpandLocalPath(Path("%LOCAL_PATH%\\terrain.xcm"));
  ok1(stored != nullptr);
  ok1(StringFind(stored.c_str(), "maps") == nullptr);

  const auto resolved = competition_map.GetPath(ProfileKeys::MapFile);
  ok1(resolved != nullptr);
  ok1(File::Exists(resolved));
  ok1(StringFind(resolved.c_str(), "maps") != nullptr);

  ok1(File::Exists(AllocatedPath::Build(data_path,
                                        Path(".xcsoar-subdir-layout-v1"))));
  ok1(File::Exists(AllocatedPath::Build(AllocatedPath::Build(data_path,
                                                             Path("cache")),
                                        Path("repository"))));
  ok1(!File::Exists(AllocatedPath::Build(data_path, Path("repository"))));
  ok1(File::Exists(AllocatedPath::Build(AllocatedPath::Build(data_path,
                                                             Path("cache")),
                                        Path("notams.json"))));
  ok1(!File::Exists(AllocatedPath::Build(data_path, Path("notams.json"))));
}

int
main()
{
  plan_tests(20);
  TestMigratesFilesAndActiveProfileOnly();
  return exit_status();
}
