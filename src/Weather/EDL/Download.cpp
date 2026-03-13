// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Download.hpp"
#include "Request.hpp"
#include "Operation/ProgressListener.hpp"
#include "net/http/CoDownloadToFile.hpp"
#include "system/FileUtil.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <map>
#include <stdio.h>

namespace EDL {

bool
CachedDay::IsComplete() const noexcept
{
  return file_count >= 24u * NUM_ISOBARS;
}

static BrokenDateTime
NormaliseDay(BrokenDateTime day) noexcept
{
  return day.AtMidnight();
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
EnsureDownloaded(BrokenDateTime forecast, unsigned isobar,
                 CurlGlobal &curl, ProgressListener &progress)
{
  auto path = BuildCachePath(forecast, isobar);
  if (!File::ExistsAny(path)) {
    const auto url = BuildDownloadUrl(forecast, isobar);
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

      const auto ignored = co_await EnsureDownloaded(forecast, isobar, curl, progress);
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
ListDownloadedDays() noexcept
{
  CachedDayVisitor visitor;
  Directory::VisitSpecificFiles(BuildCacheDirectory(), "*.mbtiles", visitor);
  return visitor.Export();
}

class DeleteOtherDaysVisitor final : public File::Visitor {
  const BrokenDateTime keep_day;
  unsigned deleted = 0;

public:
  explicit DeleteOtherDaysVisitor(BrokenDateTime _keep_day) noexcept
    :keep_day(NormaliseDay(_keep_day)) {}

  void Visit(Path path, Path filename) override
  {
    BrokenDateTime forecast;
    if (!ParseCacheFileName(filename, forecast))
      return;

    if (forecast.AtMidnight() == keep_day)
      return;

    if (File::Delete(path))
      ++deleted;
  }

  unsigned GetDeleted() const noexcept
  {
    return deleted;
  }
};

unsigned
DeleteOtherDownloadedDays(BrokenDateTime keep_day) noexcept
{
  DeleteOtherDaysVisitor visitor(keep_day);
  Directory::VisitSpecificFiles(BuildCacheDirectory(), "*.mbtiles", visitor);
  return visitor.GetDeleted();
}

} // namespace EDL
