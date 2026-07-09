// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermForecastTime.hpp"
#include "XCThermAPI.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

#include <chrono>
#include <optional>

namespace XCTherm {

UtcTimeParts
GetUtcTimeParts() noexcept
{
  UtcTimeParts utc;
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible()) {
    utc.hour = basic.date_time_utc.hour;
    utc.minute = basic.date_time_utc.minute;
  }
  return utc;
}

unsigned
PickAutoTargetUtcHour(unsigned utc_h, unsigned utc_min) noexcept
{
  return (utc_min >= 45) ? (utc_h + 1) % 24 : utc_h;
}

int
PickAutoTimeIndex(const std::vector<unsigned> &cached_hours,
                  unsigned utc_h, unsigned utc_min) noexcept
{
  if (cached_hours.empty())
    return -1;

  const unsigned target = PickAutoTargetUtcHour(utc_h, utc_min);

  int best_future = -1;
  unsigned best_future_dist = 25;
  int best_past = -1;
  unsigned best_past_dist = 25;

  for (size_t i = 0; i < cached_hours.size(); ++i) {
    const unsigned fwd = (cached_hours[i] + 24 - target) % 24;
    if (fwd <= 12) {
      if (fwd < best_future_dist) {
        best_future_dist = fwd;
        best_future = int(i);
      }
    } else {
      const unsigned back = 24 - fwd;
      if (back < best_past_dist) {
        best_past_dist = back;
        best_past = int(i);
      }
    }
  }

  return best_future >= 0 ? best_future : best_past;
}

unsigned
ForecastHourAt(const std::vector<unsigned> &cached_hours,
             unsigned time_index,
             const std::vector<unsigned> &available_hours,
             unsigned fallback) noexcept
{
  if (!available_hours.empty()) {
    if (time_index < available_hours.size())
      return available_hours[time_index];
    return available_hours[0];
  }

  if (time_index < cached_hours.size())
    return cached_hours[time_index];
  if (!cached_hours.empty())
    return cached_hours[0];

  return time_index < 24 ? time_index : fallback;
}

bool
MakeForecastDateTime(std::string_view run_date,
                     std::string_view run_hour,
                     unsigned step,
                     std::chrono::sys_seconds &out) noexcept
{
  if (run_date.size() != 8 || run_hour.size() != 2)
    return false;

  try {
    const int year = std::stoi(std::string{run_date.substr(0, 4)});
    const unsigned month = (unsigned)std::stoul(
      std::string{run_date.substr(4, 2)});
    const unsigned day = (unsigned)std::stoul(
      std::string{run_date.substr(6, 2)});
    const unsigned hour = (unsigned)std::stoul(std::string{run_hour});

    const std::chrono::year_month_day ymd{
      std::chrono::year{year},
      std::chrono::month{month},
      std::chrono::day{day},
    };
    if (!ymd.ok())
      return false;

    out = std::chrono::sys_days{ymd}
      + std::chrono::hours{hour}
      + std::chrono::hours{step};
    return true;
  } catch (...) {
    return false;
  }
}

void
FormatTimeLabel(StaticString<64> &dest, const char *api_parameter,
                unsigned forecast_utc_hour, bool time_auto_active) noexcept
{
  int offset_min = 0;
  bool has_real_offset = false;
  std::optional<XCThermAPI::SliceMeta> slice;
  if (api_parameter != nullptr && api_parameter[0] != '\0')
    slice = XCThermAPI::Instance().GetSliceMeta(api_parameter,
                                                forecast_utc_hour);

  std::chrono::sys_seconds forecast_dt{};
  if (slice.has_value() &&
      MakeForecastDateTime(slice->run_date, slice->run_hour,
                           slice->step, forecast_dt)) {
    const auto delta = forecast_dt - std::chrono::system_clock::now();
    offset_min = (int)std::chrono::duration_cast<std::chrono::minutes>(delta)
      .count();
    has_real_offset = true;
  } else if (CommonInterface::Basic().date_time_utc.IsPlausible()) {
    const auto &basic = CommonInterface::Basic();
    const int cur_min = int(basic.date_time_utc.hour) * 60
                      + int(basic.date_time_utc.minute);
    const int fc_min = int(forecast_utc_hour) * 60;
    offset_min = fc_min - cur_min;
    if (offset_min < 0)
      offset_min += 1440;
    has_real_offset = true;
  }

  char offset_buf[16] = {};
  if (has_real_offset)
    WeatherMapOverlay::FormatSignedMinuteOffset(offset_buf, sizeof(offset_buf),
                                         offset_min);

  WeatherMapOverlay::FormatAutoUtcHourLabel(dest, time_auto_active,
                                     forecast_utc_hour, offset_buf);
}

} // namespace XCTherm
