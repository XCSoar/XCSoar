// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/BackupPaths.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(10);

  ok1(IsBackupExcludedPath("cache/"));
  ok1(IsBackupExcludedPath("cache/tiles.dat"));
  ok1(IsBackupExcludedPath("xcsoar.log"));
  ok1(IsBackupExcludedPath("backup.tar"));
  ok1(!IsBackupExcludedPath("xcsoar.prf"));
  ok1(!IsBackupExcludedPath("logs/flight.igc"));

  ok1(MakeArchiveNameIfUnderRoot(Path(nullptr), Path("/data")).empty());
  ok1(MakeArchiveNameIfUnderRoot(Path("/data/xcsoar.prf"),
                                 Path("/data")) == "xcsoar.prf");
  ok1(MakeArchiveNameIfUnderRoot(Path("/data/logs/a.igc"),
                                 Path("/data")) == "logs/a.igc");
  ok1(MakeArchiveNameIfUnderRoot(Path("/other/a.igc"),
                                 Path("/data")).empty());

  return exit_status();
}
