// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightCache.hpp"

#include "Interface.hpp"
#include "system/FileUtil.hpp"
#include "time/BrokenDateTime.hpp"

#if defined(__linux__) && defined(USE_POLL_EVENT) && !defined(KOBO)
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/TimeDate.hxx"
#endif

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <utility>

namespace {

constexpr auto FORECAST_RETENTION = std::chrono::hours{12};

[[nodiscard]] bool
IsGpsTimeValidForForecastCleanup() noexcept
{
  const auto &basic = CommonInterface::Basic();
  return basic.gps.real && basic.time_available &&
    basic.date_time_utc.IsDatePlausible();
}

#if defined(__linux__) && defined(USE_POLL_EVENT) && !defined(KOBO)
[[nodiscard]] bool
HasNtpSynchronizedSystemTimeForForecastCleanup() noexcept
{
  try {
    auto connection = ODBus::Connection::GetSystem();
    return TimeDate::IsNTPSynchronized(connection);
  } catch (...) {
    return false;
  }
}
#endif

template<typename V>
void
VisitForecastImageFiles(Path directory, V &visitor)
{
  Directory::VisitSpecificFiles(directory, "*.tif", visitor);
  Directory::VisitSpecificFiles(directory, "*.tiff", visitor);
  Directory::VisitSpecificFiles(directory, "*.png", visitor);
  Directory::VisitSpecificFiles(directory, "*.jpg", visitor);
  Directory::VisitSpecificFiles(directory, "*.jpeg", visitor);
}

[[nodiscard]] bool
StripSuffix(std::string_view &value, std::string_view suffix) noexcept
{
  if (!value.ends_with(suffix))
    return false;

  value.remove_suffix(suffix.size());
  return true;
}

[[nodiscard]] std::string_view
ExtractTimestampPrefix(std::string_view stem) noexcept
{
  if (stem.size() < 16 || stem[stem.size() - 16] != '-')
    return {};

  return stem.substr(0, stem.size() - 16);
}

[[nodiscard]] bool
IsUnsignedNumber(std::string_view value) noexcept
{
  return !value.empty() &&
    std::all_of(value.begin(), value.end(), [](char ch) {
      return ch >= '0' && ch <= '9';
    });
}

[[nodiscard]] bool
LooksLikeTileCacheStem(std::string_view stem) noexcept
{
  auto prefix = ExtractTimestampPrefix(stem);
  if (prefix.empty())
    return false;

  const auto split3 = prefix.rfind('-');
  if (split3 == std::string_view::npos ||
      !IsUnsignedNumber(prefix.substr(split3 + 1)))
    return false;

  prefix = prefix.substr(0, split3);
  const auto split2 = prefix.rfind('-');
  if (split2 == std::string_view::npos ||
      !IsUnsignedNumber(prefix.substr(split2 + 1)))
    return false;

  prefix = prefix.substr(0, split2);
  const auto split1 = prefix.rfind('-');
  if (split1 == std::string_view::npos ||
      !IsUnsignedNumber(prefix.substr(split1 + 1)))
    return false;

  return true;
}

[[nodiscard]] std::string_view
StripForecastArtifactSuffix(std::string_view filename) noexcept
{
  auto stem = filename;

  if (StripSuffix(stem, ".min")) {
    if (StripSuffix(stem, ".nc") ||
        StripSuffix(stem, ".tif") ||
        StripSuffix(stem, ".tiff") ||
        StripSuffix(stem, ".png"))
      return stem;

    return {};
  }

  if (StripSuffix(stem, ".zip") ||
      StripSuffix(stem, ".nc") ||
    StripSuffix(stem, ".jpg") ||
      StripSuffix(stem, ".tif") ||
      StripSuffix(stem, ".tiff") ||
      StripSuffix(stem, ".png") ||
      StripSuffix(stem, ".jpeg"))
    return stem;

  return {};
}

[[nodiscard]] time_t
ParseAnyForecastFileTimestamp(std::string_view filename) noexcept
{
  const bool is_jpg = filename.ends_with(".jpg");
  auto stem = StripForecastArtifactSuffix(filename);
  if (stem.size() < 16)
    return 0;

  if (is_jpg && LooksLikeTileCacheStem(stem))
    return 0;

  const auto timestamp = stem.substr(stem.size() - 15);
  if (stem[stem.size() - 16] != '-')
    return 0;

  unsigned year, month, day, hour, minute;
  if (std::sscanf(std::string{timestamp}.c_str(), "%4u-%2u-%2u-%2u%2u",
                  &year, &month, &day, &hour, &minute) != 5)
    return 0;

  const BrokenDateTime date_time{year, month, day, hour, minute};
  if (!date_time.IsPlausible())
    return 0;

  return std::chrono::system_clock::to_time_t(date_time.ToTimePoint());
}
[[nodiscard]] time_t
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

class OlderThanForecastTimeVisitor final : public File::Visitor {
  const time_t cutoff;
  const std::chrono::system_clock::time_point fallback_cutoff;

public:
  explicit OlderThanForecastTimeVisitor(time_t _cutoff) noexcept
    :cutoff(_cutoff),
     fallback_cutoff(std::chrono::system_clock::from_time_t(_cutoff)) {}

  void Visit(Path full_path, Path filename) override {
    const auto forecast_time = ParseAnyForecastFileTimestamp(filename.c_str());
    if (forecast_time > 0) {
      if (forecast_time < cutoff)
        File::Delete(full_path);

      return;
    }

    if (File::GetLastModification(full_path) < fallback_cutoff)
      File::Delete(full_path);
  }
};

} // namespace

namespace SkySightCache {

/**
 * Returns true only when GPS time is valid or the supported host reports NTP
 * synchronization, allowing age-based forecast cleanup to be trusted.
 */
bool
IsTrustedTimeAvailableForCleanup() noexcept
{
  if (IsGpsTimeValidForForecastCleanup())
    return true;

#if defined(__linux__) && defined(USE_POLL_EVENT) && !defined(KOBO)
  return HasNtpSynchronizedSystemTimeForForecastCleanup();
#else
  return false;
#endif
}

/**
 * Selects an exact preferred forecast first, otherwise the newest forecast not
 * later than now, and finally the earliest future forecast.
 */
ForecastImageCandidate
FindForecastImage(Path directory, std::string_view region,
                  std::string_view layer_id,
                  time_t preferred_time)
{
  const auto now = std::time(nullptr);
  std::string prefix{region};
  prefix += '-';
  prefix += layer_id;
  prefix += '-';

  struct Visitor final : public File::Visitor {
    const std::string_view prefix;
    const time_t preferred_time;
    const time_t now;
    ForecastImageCandidate exact;
    ForecastImageCandidate latest_past;
    ForecastImageCandidate earliest_future;
    std::chrono::system_clock::time_point exact_mtime{};
    std::chrono::system_clock::time_point latest_past_mtime{};
    std::chrono::system_clock::time_point earliest_future_mtime{};

    Visitor(std::string_view _prefix, time_t _preferred_time,
            time_t _now) noexcept
      :prefix(_prefix),
       preferred_time(_preferred_time),
       now(_now) {}

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
  } visitor(prefix, preferred_time, now);

  VisitForecastImageFiles(directory, visitor);

  if (visitor.exact.path != nullptr)
    return std::move(visitor.exact);

  if (visitor.latest_past.path != nullptr)
    return std::move(visitor.latest_past);

  return std::move(visitor.earliest_future);
}

/**
 * Collects unique cached forecast timestamps for one region and layer, ordered
 * from newest to oldest.
 */
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

/**
 * Removes forecasts older than the retention window, stale raw/intermediate
 * artifacts using the same cutoff, and short-lived temporary metadata files.
 */
void
Cleanup(Path directory) noexcept
{
  try {
    const auto now = std::chrono::system_clock::now();
    OlderThanFileVisitor delete_tmp{now - std::chrono::hours{6}};
    OlderThanFileVisitor delete_json{now - std::chrono::hours{1}};

    if (IsTrustedTimeAvailableForCleanup()) {
      OlderThanForecastTimeVisitor delete_forecasts{
        std::chrono::system_clock::to_time_t(now - FORECAST_RETENTION)};
      VisitForecastImageFiles(directory, delete_forecasts);
      Directory::VisitSpecificFiles(directory, "*.nc", delete_forecasts);
      Directory::VisitSpecificFiles(directory, "*.min", delete_forecasts);
      Directory::VisitSpecificFiles(directory, "*.zip", delete_forecasts);
    }

    Directory::VisitSpecificFiles(directory, "*.tmp", delete_tmp);
    Directory::VisitSpecificFiles(directory, "*.json", delete_json);
  } catch (...) {
  }
}

} // namespace SkySightCache
