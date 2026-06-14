// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/EDL/TileStore.hpp"
#include "Weather/EDL/TileValue.hpp"
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

static void
TestTrackedForecastTime()
{
  const BrokenDateTime utc(2026, 3, 12, 14, 37, 0);
  const auto tracked = EDL::GetTrackedForecastTime(utc);
  const BrokenDateTime expected(2026, 3, 12, 15, 0, 0);

  if (!ok1(tracked == expected))
    diag("tracked forecast mismatch: expected %04u-%02u-%02u %02u:00, got %04u-%02u-%02u %02u:00",
         expected.year, expected.month, expected.day, expected.hour,
         tracked.year, tracked.month, tracked.day, tracked.hour);
}

static void
TestAscendancyPixelDecode()
{
  double value = 0;

  ok1(!EDL::DecodeAscendancyPixel(0, 0, 0, 0, value));

  ok1(EDL::DecodeAscendancyPixel(0, 0, 0, 128, value));
  ok1(equals(value, 0.0196078431372549));

  ok1(EDL::DecodeAscendancyPixel(0, 255, 0, 255, value));
  ok1(equals(value, 5.));

  ok1(EDL::DecodeAscendancyPixel(255, 0, 0, 255, value));
  ok1(equals(value, -5.));

  ok1(EDL::DecodeAscendancyPixel(255, 255, 0, 6, value));
  ok1(equals(value, 4.764705882352941));

  ok1(EDL::DecodeAscendancyPixel(255, 116, 0, 153, value));
  ok1(equals(value, 5.));

  ok1(EDL::DecodeAscendancyPixel(0, 255, 255, 66, value));
  ok1(equals(value, -2.411764705882353));

  ok1(EDL::DecodeAscendancyPixel(127, 127, 0, 255, value));
  ok1(equals(value, 2.490196078431373));

  ok1(EDL::DecodeAscendancyPixel(0, 0, 255, 255, value));
  ok1(equals(value, -5.));
}

int
main()
{
  plan_tests(4 + 12);

  TestRequest();
  TestTrackedForecastTime();
  TestAscendancyPixelDecode();

  return exit_status();
}
