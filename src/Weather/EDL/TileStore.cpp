// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TileStore.hpp"
#include "LocalPath.hpp"
#include "io/DirectoryUtil.hpp"
#include "Operation/ProgressListener.hpp"
#include "system/FileUtil.hpp"
#include "net/http/CoDownloadToFile.hpp"

#include <algorithm>
#include <map>
#include <stdio.h>

namespace EDL {

static constexpr char BASE_URL[] =
  "https://www.edl-soaring.com/mbtiles/extract_mbtiles_from_date.php";

BrokenDateTime
NormaliseForecastHour(BrokenDateTime forecast) noexcept
{
  return BrokenDateTime(forecast.year, forecast.month, forecast.day,
                        forecast.hour, 0, 0);
}

bool
CachedDay::IsComplete() const noexcept
{
  return file_count >= 24u * NUM_ISOBARS;
}

StaticString<32>
TileRequest::BuildForecastParameter() const noexcept
{
  StaticString<32> parameter;
  parameter.Format("%04u-%02u-%02u+%02u:%02u:%02u",
                   forecast.year, forecast.month, forecast.day,
                   forecast.hour, forecast.minute, forecast.second);
  return parameter;
}

StaticString<256>
TileRequest::BuildDownloadUrl() const noexcept
{
  const auto date_parameter = BuildForecastParameter();

  StaticString<256> url;
  url.Format("%s?fdate=%s&isobare=%u",
             BASE_URL, date_parameter.c_str(), isobar);
  return url;
}

StaticString<64>
TileRequest::BuildCacheFileName() const noexcept
{
  StaticString<64> filename;
  filename.Format("%04u%02u%02u_%02u%02u%02u_%u.mbtiles",
                  forecast.year, forecast.month, forecast.day,
                  forecast.hour, forecast.minute, forecast.second,
                  isobar);
  return filename;
}

AllocatedPath
BuildCacheDirectory()
{
  auto path = MakeCacheDirectory("edl");
  path = AllocatedPath::Build(path, Path("mbtiles"));
  Directory::Create(path);
  return path;
}

AllocatedPath
TileRequest::BuildCachePath() const
{
  const auto mbtiles_path = BuildCacheDirectory();

  const auto filename = BuildCacheFileName();
  return AllocatedPath::Build(mbtiles_path, Path(filename.c_str()));
}

static BrokenDateTime
NormaliseDay(BrokenDateTime day) noexcept
{
  return NormaliseForecastHour(day).AtMidnight();
}

static bool
ParseCacheFileName(Path filename, BrokenDateTime &forecast) noexcept
{
  unsigned year, month, day, hour, minute, second, isobar;
  if (sscanf(filename.c_str(), "%4u%2u%2u_%2u%2u%2u_%u.mbtiles",
             &year, &month, &day, &hour, &minute, &second, &isobar) != 7)
    return false;

  const BrokenDateTime parsed(year, month, day, hour, minute, second);
  if (!parsed.IsPlausible() || !IsSupportedIsobar(isobar))
    return false;

  forecast = parsed;
  return true;
}

Co::Task<AllocatedPath>
TileRequest::EnsureDownloaded(CurlGlobal &curl,
                              ProgressListener &progress) const
{
  auto path = BuildCachePath();
  if (!File::ExistsAny(path)) {
    const auto url = BuildDownloadUrl();
    const auto ignored = co_await Net::CoDownloadToFile(curl, url.c_str(),
                                                        nullptr, nullptr,
                                                        path, nullptr,
                                                        progress);
    (void)ignored;
  }

  co_return path;
}

Co::Task<unsigned>
EnsureDayDownloaded(BrokenDateTime day, CurlGlobal &curl,
                    ProgressListener &progress)
{
  const auto day_utc = NormaliseDay(day);

  progress.SetProgressRange(24u * NUM_ISOBARS);

  unsigned position = 0;
  for (unsigned hour = 0; hour < 24; ++hour) {
    BrokenDateTime forecast(day_utc.year, day_utc.month, day_utc.day, hour, 0, 0);

    for (unsigned isobar : ISOBARS) {
      progress.SetProgressPosition(position++);

      const auto ignored =
        co_await TileRequest(forecast, isobar).EnsureDownloaded(curl, progress);
      (void)ignored;
    }
  }

  progress.SetProgressPosition(position);
  co_return position;
}

class CachedDayVisitor final : public File::Visitor {
  std::map<BrokenDateTime, unsigned> counts;

public:
  void Visit(Path, Path filename) override
  {
    BrokenDateTime forecast;
    if (!ParseCacheFileName(filename, forecast))
      return;

    ++counts[forecast.AtMidnight()];
  }

  std::vector<CachedDay> Export() const
  {
    std::vector<CachedDay> days;
    days.reserve(counts.size());

    for (const auto &[day, file_count] : counts)
      days.push_back({day, file_count});

    std::reverse(days.begin(), days.end());
    return days;
  }
};

std::vector<CachedDay>
ListDownloadedDays()
{
  CachedDayVisitor visitor;
  DirectoryUtil::VisitSpecificFiles(BuildCacheDirectory(), "*.mbtiles",
                                    [&visitor](Path path, Path filename) {
                                      visitor.Visit(path, filename);
                                    });
  return visitor.Export();
}

unsigned
DeleteOtherDownloadedDays(BrokenDateTime keep_day)
{
  const auto keep_day_utc = NormaliseDay(keep_day);

  return DirectoryUtil::DeleteSpecificFilesExcept(
    BuildCacheDirectory(), "*.mbtiles",
    [keep_day_utc](Path, Path filename) {
      BrokenDateTime forecast;
      if (!ParseCacheFileName(filename, forecast))
        return true;

      return forecast.AtMidnight() == keep_day_utc;
    });
}

} // namespace EDL
