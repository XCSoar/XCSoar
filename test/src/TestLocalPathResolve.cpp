// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalPath.hpp"
#include "Repository/FileType.hpp"
#include "Task/DefaultTask.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"

#include <cstdio>

static bool
TouchFile(const AllocatedPath &path) noexcept
{
  FileOutputStream out(path, FileOutputStream::Mode::CREATE);
  out.Commit();
  return true;
}

static void
TestLayoutSubdirForFilename()
{
  ok1(GetLayoutSubdirForFilename("terrain.xcm") == Path("maps"));
  ok1(GetLayoutSubdirForFilename("user.cup") == Path("waypoints"));
  ok1(GetLayoutSubdirForFilename("repository") == Path("cache"));
  ok1(GetLayoutSubdirForFilename("user_repository_3") == Path("cache"));
  ok1(GetLayoutSubdirForFilename("notams.json") == Path("cache"));
  ok1(IsCacheLayoutFilename("repository"));
  ok1(IsCacheLayoutFilename("notams.json"));
  ok1(!IsCacheLayoutFilename("terrain.xcm"));
  ok1(GetLayoutSubdirForFilename("xcsoar.log") == Path("logs"));
  ok1(GetLayoutSubdirForFilename("flights.log") == Path("logs"));
  ok1(GetLayoutSubdirForFilename("foo.json") == nullptr);
}

static void
TestResolveTypedDataFilePath()
{
  char template_path[] = "/tmp/xcsoar-resolve-XXXXXX";
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));

  const auto root = LocalPath("user.cup");
  const auto subdir =
    TypedDataSavePath(FileType::WAYPOINT, "user.cup");

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
  const Path data_path(template_path);
  SetSingleDataPath(data_path);

  const auto in_cache = CacheDataSavePath("repository");
  const auto in_root = LocalPath("repository");

  TouchFile(in_cache);
  ok1(ResolveRepositoryDataPath("repository") == in_cache);

  File::Delete(in_cache);
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
  const auto in_root = LocalPath("notams.json");

  TouchFile(in_cache);
  ok1(ResolveCacheDataPath("notams.json") == in_cache);

  File::Delete(in_cache);
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

  const auto save_path = TypedDataSavePath(FileType::TASK, default_task_path);
  ok1(StringFind(save_path.c_str(), "tasks") != nullptr);
  ok1(StringFind(save_path.c_str(), default_task_path) != nullptr);

  const auto legacy =
    TypedDataSavePath(FileType::TASK, "Default.tsk");
  TouchFile(legacy);
  const auto resolved =
    ResolveTypedDataFilePath(FileType::TASK, default_task_path,
                             {"Default.tsk"});
  ok1(resolved == legacy);

  File::Delete(legacy);
  TouchFile(save_path);
  ok1(ResolveTypedDataFilePath(FileType::TASK, default_task_path,
                               {"Default.tsk"}) == save_path);
}

static void
TestRepositoryDownloadRelativePath()
{
  const auto relative = RepositoryDownloadRelativePath("repository");
  ok1(StringFind(relative.c_str(), "cache") != nullptr);
  ok1(StringFind(relative.c_str(), "repository") != nullptr);

  /* Same pattern as DownloadManager queue: Path must outlive the copy. */
  Path path_copy(relative.c_str());
  const AllocatedPath stored(path_copy);
  ok1(StringIsEqual(stored.c_str(), relative.c_str()));
}

int
main()
{
  plan_tests(33);
  TestLayoutSubdirForFilename();
  TestResolveTypedDataFilePath();
  TestResolveRepositoryDataPath();
  TestResolveCacheDataPath();
  TestResolveLogsDataPath();
  TestDefaultTaskPaths();
  TestRepositoryDownloadRelativePath();
  return exit_status();
}
