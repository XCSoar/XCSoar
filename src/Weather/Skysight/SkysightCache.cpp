// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightCache.hpp"

#include "system/FileUtil.hpp"
#include "time/BrokenDateTime.hpp"

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
                  std::string_view layer_id)
{
  ForecastImageCandidate best;
  std::chrono::system_clock::time_point best_mtime{};
  std::string prefix{region};
  prefix += '-';
  prefix += layer_id;
  prefix += '-';

  struct Visitor final : public File::Visitor {
    const std::string_view prefix;
    ForecastImageCandidate &best;
    std::chrono::system_clock::time_point &best_mtime;

    Visitor(std::string_view _prefix, ForecastImageCandidate &_best,
            std::chrono::system_clock::time_point &_best_mtime) noexcept
      :prefix(_prefix), best(_best), best_mtime(_best_mtime) {}

    void Visit(Path full_path, Path filename) override {
      const auto forecast_time =
        ParseForecastFileTimestamp(filename.c_str(), prefix);
      if (forecast_time <= 0)
        return;

      const auto mtime = File::GetLastModification(full_path);
      if (best.path != nullptr &&
          (forecast_time < best.forecast_time ||
           (forecast_time == best.forecast_time && mtime <= best_mtime)))
        return;

      best.path = AllocatedPath(full_path.c_str());
      best.forecast_time = forecast_time;
      best_mtime = mtime;
    }
  } visitor(prefix, best, best_mtime);

  VisitForecastImageFiles(directory, visitor);
  return best;
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
