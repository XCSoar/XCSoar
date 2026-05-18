// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/EDL/TileStore.hpp"
#include "TestUtil.hpp"
#include "time/BrokenDateTime.hpp"

#include <cstring>

static void
CheckStringEquals(const char *label, const char *actual,
                  const char *expected) noexcept
{
  if (!ok1(strcmp(actual, expected) == 0))
    diag("%s mismatch: expected \"%s\", got \"%s\"",
         label, expected, actual);
}

static void
TestRequest()
{
  const BrokenDateTime forecast(2026, 3, 12, 6, 0, 0);
  const EDL::TileRequest request(forecast, 90000);

  const auto parameter = request.BuildForecastParameter();
  CheckStringEquals("forecast parameter", parameter.c_str(),
                    "2026-03-12+06:00:00");

  const auto url = request.BuildDownloadUrl();
  CheckStringEquals("download URL", url.c_str(),
                    "https://www.edl-soaring.com/mbtiles/extract_mbtiles_from_date.php?fdate=2026-03-12+06:00:00&isobare=90000");

  const auto filename = EDL::TileRequest(forecast, 70000).BuildCacheFileName();
  CheckStringEquals("cache filename", filename.c_str(),
                    "20260312_060000_70000.mbtiles");
}

int
main()
{
  plan_tests(3);

  TestRequest();

  return exit_status();
}
