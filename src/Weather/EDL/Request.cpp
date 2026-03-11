// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Request.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"

#include <stdio.h>

namespace EDL {

static constexpr char BASE_URL[] =
  "https://www.edl-soaring.com/mbtiles/extract_mbtiles_from_date.php";

bool
IsSupportedIsobar(unsigned isobar) noexcept
{
  for (unsigned value : ISOBARS)
    if (value == isobar)
      return true;

  return false;
}

void
FormatForecastParameter(char *buffer, const BrokenDateTime &forecast) noexcept
{
  sprintf(buffer, "%04u-%02u-%02u+%02u:%02u:%02u",
          forecast.year, forecast.month, forecast.day,
          forecast.hour, forecast.minute, forecast.second);
}

StaticString<256>
BuildDownloadUrl(const BrokenDateTime &forecast, unsigned isobar) noexcept
{
  StaticString<32> date_parameter;
  FormatForecastParameter(date_parameter.buffer(), forecast);

  StaticString<256> url;
  url.Format("%s?fdate=%s&isobare=%u",
             BASE_URL, date_parameter.c_str(), isobar);
  return url;
}

StaticString<64>
BuildCacheFileName(const BrokenDateTime &forecast, unsigned isobar) noexcept
{
  StaticString<64> filename;
  filename.Format("%04u%02u%02u_%02u%02u%02u_%u.mbtiles",
                  forecast.year, forecast.month, forecast.day,
                  forecast.hour, forecast.minute, forecast.second,
                  isobar);
  return filename;
}

AllocatedPath
BuildCachePath(const BrokenDateTime &forecast, unsigned isobar) noexcept
{
  const auto weather_path = LocalPath("weather");
  Directory::Create(weather_path);
  const auto edl_path = AllocatedPath::Build(weather_path, Path("edl"));
  Directory::Create(edl_path);
  const auto mbtiles_path = AllocatedPath::Build(edl_path, Path("mbtiles"));
  Directory::Create(mbtiles_path);

  const auto filename = BuildCacheFileName(forecast, isobar);
  return AllocatedPath::Build(mbtiles_path, Path(filename.c_str()));
}

} // namespace EDL
