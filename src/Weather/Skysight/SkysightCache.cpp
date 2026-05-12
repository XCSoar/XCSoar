// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightCache.hpp"

#include "system/FileUtil.hpp"
#include "time/BrokenDateTime.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>

namespace {

template<typename V>
static void
VisitForecastImageFiles(Path directory, V &visitor)
{
  Directory::VisitSpecificFiles(directory, "*.tif", visitor);
  Directory::VisitSpecificFiles(directory, "*.tiff", visitor);
  Directory::VisitSpecificFiles(directory, "*.png", visitor);
  Directory::VisitSpecificFiles(directory, "*.jpg", visitor);
  Directory::VisitSpecificFiles(directory, "*.jpeg", visitor);
}

[[nodiscard]] static time_t
ParseForecastFileTimestamp(std::string_view filename,
                           std::string_view prefix) noexcept
{
  if (filename.size() <= prefix.size() ||
      filename.compare(0, prefix.size(), prefix) != 0)
    return 0;

  const auto dot = filename.find_last_of('.');
  if (dot == std::string_view::npos || dot <= prefix.size())
    return 0;

  unsigned year, month, day, hour, minute;
  const std::string timestamp{filename.substr(prefix.size(), dot - prefix.size())};
  if (std::sscanf(timestamp.c_str(), "%4u-%2u-%2u-%2u%2u",
                  &year, &month, &day, &hour, &minute) != 5)
    return 0;

  const BrokenDateTime date_time{year, month, day, hour, minute};
  if (!date_time.IsPlausible())
    return 0;

  return std::chrono::system_clock::to_time_t(date_time.ToTimePoint());
}

class OlderThanFileVisitor final : public File::Visitor {
  const std::chrono::system_clock::time_point cutoff;

public:
  explicit OlderThanFileVisitor(std::chrono::system_clock::time_point _cutoff) noexcept
    :cutoff(_cutoff) {}

  void Visit(Path full_path, [[maybe_unused]] Path filename) override {
    if (File::GetLastModification(full_path) < cutoff)
      File::Delete(full_path);
  }
};

} // namespace

namespace SkysightCache {

ForecastImageCandidate
FindForecastImage(Path directory, std::string_view region,
                  std::string_view layer_id,
                  time_t preferred_time)
{
  ForecastImageCandidate exact;
  ForecastImageCandidate latest_past;
  ForecastImageCandidate earliest_future;
  std::chrono::system_clock::time_point exact_mtime{};
  std::chrono::system_clock::time_point latest_past_mtime{};
  std::chrono::system_clock::time_point earliest_future_mtime{};
  const auto now = std::time(nullptr);
  std::string prefix{region};
  prefix += '-';
  prefix += layer_id;
  prefix += '-';

  struct Visitor final : public File::Visitor {
    const std::string_view prefix;
    const time_t preferred_time;
    const time_t now;
    ForecastImageCandidate &exact;
    ForecastImageCandidate &latest_past;
    ForecastImageCandidate &earliest_future;
    std::chrono::system_clock::time_point &exact_mtime;
    std::chrono::system_clock::time_point &latest_past_mtime;
    std::chrono::system_clock::time_point &earliest_future_mtime;

    Visitor(std::string_view _prefix, time_t _preferred_time, time_t _now,
            ForecastImageCandidate &_exact,
            ForecastImageCandidate &_latest_past,
            ForecastImageCandidate &_earliest_future,
            std::chrono::system_clock::time_point &_exact_mtime,
            std::chrono::system_clock::time_point &_latest_past_mtime,
            std::chrono::system_clock::time_point &_earliest_future_mtime) noexcept
      :prefix(_prefix),
       preferred_time(_preferred_time),
       now(_now),
       exact(_exact),
       latest_past(_latest_past),
       earliest_future(_earliest_future),
       exact_mtime(_exact_mtime),
       latest_past_mtime(_latest_past_mtime),
       earliest_future_mtime(_earliest_future_mtime) {}

    void Visit(Path full_path, Path filename) override {
      const auto forecast_time =
        ParseForecastFileTimestamp(filename.c_str(), prefix);
      if (forecast_time <= 0)
        return;

      const auto mtime = File::GetLastModification(full_path);
      if (preferred_time > 0 && forecast_time == preferred_time) {
        if (exact.path == nullptr || mtime > exact_mtime) {
          exact.path = AllocatedPath(full_path.c_str());
          exact.forecast_time = forecast_time;
          exact_mtime = mtime;
        }

        return;
      }

      if (forecast_time <= now) {
        if (latest_past.path == nullptr ||
            forecast_time > latest_past.forecast_time ||
            (forecast_time == latest_past.forecast_time &&
             mtime > latest_past_mtime)) {
          latest_past.path = AllocatedPath(full_path.c_str());
          latest_past.forecast_time = forecast_time;
          latest_past_mtime = mtime;
        }
      } else if (earliest_future.path == nullptr ||
                 forecast_time < earliest_future.forecast_time ||
                 (forecast_time == earliest_future.forecast_time &&
                  mtime > earliest_future_mtime)) {
        earliest_future.path = AllocatedPath(full_path.c_str());
        earliest_future.forecast_time = forecast_time;
        earliest_future_mtime = mtime;
      }
    }
  } visitor(prefix, preferred_time, now,
            exact, latest_past, earliest_future,
            exact_mtime, latest_past_mtime, earliest_future_mtime);

  VisitForecastImageFiles(directory, visitor);

  if (exact.path != nullptr)
    return exact;

  if (latest_past.path != nullptr)
    return latest_past;

  return earliest_future;
}

std::vector<time_t>
CollectForecastTimes(Path directory, std::string_view region,
                     std::string_view layer_id)
{
  std::vector<time_t> times;

  std::string prefix{region};
  prefix += '-';
  prefix += layer_id;
  prefix += '-';

  struct Visitor final : public File::Visitor {
    const std::string_view prefix;
    std::vector<time_t> &times;

    Visitor(std::string_view _prefix, std::vector<time_t> &_times) noexcept
      :prefix(_prefix), times(_times) {}

    void Visit([[maybe_unused]] Path full_path, Path filename) override {
      const auto forecast_time =
        ParseForecastFileTimestamp(filename.c_str(), prefix);
      if (forecast_time <= 0)
        return;

      times.push_back(forecast_time);
    }
  } visitor(prefix, times);

  VisitForecastImageFiles(directory, visitor);

  std::sort(times.begin(), times.end());
  times.erase(std::unique(times.begin(), times.end()), times.end());
  std::reverse(times.begin(), times.end());
  return times;
}

void
Cleanup(Path directory) noexcept
{
  const auto now = std::chrono::system_clock::now();
  OlderThanFileVisitor delete_tiles{now - std::chrono::hours{12}};
  OlderThanFileVisitor delete_tmp{now - std::chrono::hours{6}};
  OlderThanFileVisitor delete_json{now - std::chrono::hours{1}};

  Directory::VisitSpecificFiles(directory, "*.jpg", delete_tiles);
  Directory::VisitSpecificFiles(directory, "*.tmp", delete_tmp);
  Directory::VisitSpecificFiles(directory, "*.json", delete_json);
}

} // namespace SkysightCache
