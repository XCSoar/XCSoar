// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightCache.hpp"
#include "SkySightPayloadSuffixes.hpp"

#include "Interface.hpp"
#include "system/FileUtil.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StringCompare.hxx"

#if defined(__linux__) && defined(USE_POLL_EVENT) && !defined(KOBO)
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/TimeDate.hxx"
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <utility>

using namespace std::string_view_literals;

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
  /* Decoded NetCDF overlays use the versioned .v2.tif suffix.  Plain *.tif
     files from earlier decoders are ignored so near-zero washes are not
     selected as the active overlay. */
  for (const auto glob : SkySight::DISPLAY_IMAGE_GLOBS)
    Directory::VisitSpecificFiles(directory, glob.data(), visitor);
}

[[nodiscard]] std::string
MakeForecastCachePrefix(std::string_view region,
                        std::string_view layer_id)
{
  std::string prefix;
  prefix.reserve(region.size() + layer_id.size() + 2);
  prefix += region;
  prefix += '-';
  prefix += layer_id;
  prefix += '-';
  return prefix;
}

[[nodiscard]] bool
IsUnsignedNumber(std::string_view value) noexcept
{
  return !value.empty() &&
    std::all_of(value.begin(), value.end(), [](char ch) {
      return ch >= '0' && ch <= '9';
    });
}

/**
 * Live tiles are named "{layer}-{zoom}-{x}-{y}-{YYYY-MM-DD-HHMM}.jpg".
 * Detect that pattern so cleanup does not treat them as forecasts.
 */
[[nodiscard]] bool
LooksLikeTileCacheStem(std::string_view stem) noexcept
{
  /* Drop "-YYYY-MM-DD-HHMM" (15 chars + separator). */
  if (stem.size() < 16 || stem[stem.size() - 16] != '-')
    return false;

  auto rest = stem.substr(0, stem.size() - 16);

  const auto y_split = rest.rfind('-');
  if (y_split == std::string_view::npos ||
      !IsUnsignedNumber(rest.substr(y_split + 1)))
    return false;

  rest = rest.substr(0, y_split);
  const auto x_split = rest.rfind('-');
  if (x_split == std::string_view::npos ||
      !IsUnsignedNumber(rest.substr(x_split + 1)))
    return false;

  rest = rest.substr(0, x_split);
  const auto zoom_split = rest.rfind('-');
  if (zoom_split == std::string_view::npos ||
      !IsUnsignedNumber(rest.substr(zoom_split + 1)))
    return false;

  return true;
}

[[nodiscard]] bool
ParseTimestampNumber(std::string_view text, unsigned &value) noexcept
{
  const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                      value);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

[[nodiscard]] time_t
ParseTimestamp(std::string_view timestamp) noexcept
{
  if (timestamp.size() != 15 || timestamp[4] != '-' ||
      timestamp[7] != '-' || timestamp[10] != '-')
    return 0;

  unsigned year, month, day, hour, minute;
  if (!ParseTimestampNumber(timestamp.substr(0, 4), year) ||
      !ParseTimestampNumber(timestamp.substr(5, 2), month) ||
      !ParseTimestampNumber(timestamp.substr(8, 2), day) ||
      !ParseTimestampNumber(timestamp.substr(11, 2), hour) ||
      !ParseTimestampNumber(timestamp.substr(13, 2), minute))
    return 0;

  const BrokenDateTime date_time{year, month, day, hour, minute};
  if (!date_time.IsPlausible())
    return 0;

  return std::chrono::system_clock::to_time_t(date_time.ToTimePoint());
}

[[nodiscard]] time_t
ParseAnyForecastFileTimestamp(std::string_view filename) noexcept
{
  const bool is_jpg = filename.ends_with(".jpg");
  const auto stem = SkySight::StripForecastArtifactSuffix(filename);
  if (stem.size() < 16)
    return 0;

  if (is_jpg && LooksLikeTileCacheStem(stem))
    return 0;

  if (stem[stem.size() - 16] != '-')
    return 0;

  return ParseTimestamp(stem.substr(stem.size() - 15));
}

[[nodiscard]] time_t
ParseForecastFileTimestamp(std::string_view filename,
                           std::string_view prefix) noexcept
{
  if (!filename.starts_with(prefix))
    return 0;

  auto stem = SkySight::StripForecastArtifactSuffix(filename);
  if (!SkipPrefix(stem, prefix))
    return 0;

  return ParseTimestamp(stem);
}

class OlderThanFileVisitor final : public File::Visitor {
  const std::chrono::system_clock::time_point cutoff;

public:
  explicit OlderThanFileVisitor(
      std::chrono::system_clock::time_point _cutoff) noexcept
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

Usage
GetUsage(Path directory) noexcept
{
  struct Visitor final : Directory::DirEntryVisitor {
    Usage usage;

    void Visit(Path path, [[maybe_unused]] Path filename,
               bool is_directory) noexcept override {
      if (is_directory)
        return;

      usage.bytes += File::GetSize(path);
      ++usage.files;
    }
  } visitor;

  Directory::VisitDirectoriesAndFiles(directory, visitor);
  return visitor.usage;
}

Usage
ClearDownloadedData(Path directory) noexcept
{
  struct Visitor final : Directory::DirEntryVisitor {
    Usage deleted;

    void Visit(Path path, Path filename,
               bool is_directory) noexcept override {
      if (is_directory ||
          std::string_view{filename.c_str()}.ends_with(".cache"))
        return;

      const uint64_t size = File::GetSize(path);
      if (File::Delete(path)) {
        deleted.bytes += size;
        ++deleted.files;
      }
    }
  } visitor;

  Directory::VisitDirectoriesAndFiles(directory, visitor);
  return visitor.deleted;
}

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
  const auto prefix = MakeForecastCachePrefix(region, layer_id);

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
  const auto prefix = MakeForecastCachePrefix(region, layer_id);

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
bool
Cleanup(Path directory) noexcept
{
  const bool trusted_time = IsTrustedTimeAvailableForCleanup();

  try {
    const auto now = std::chrono::system_clock::now();
    OlderThanFileVisitor delete_tmp{now - std::chrono::hours{6}};
    OlderThanFileVisitor delete_json{now - std::chrono::hours{1}};

    if (trusted_time) {
      OlderThanForecastTimeVisitor delete_forecasts{
        std::chrono::system_clock::to_time_t(now - FORECAST_RETENTION)};
      VisitForecastImageFiles(directory, delete_forecasts);
      for (const auto glob : SkySight::RAW_FORECAST_GLOBS)
        Directory::VisitSpecificFiles(directory, glob.data(),
                                      delete_forecasts);
    }

    Directory::VisitSpecificFiles(directory, "*.tmp", delete_tmp);
    Directory::VisitSpecificFiles(directory, "*.json", delete_json);
  } catch (...) {
  }

  return trusted_time;
}

} // namespace SkySightCache
